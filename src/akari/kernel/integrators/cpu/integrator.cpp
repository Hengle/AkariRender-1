// MIT License
//
// Copyright (c) 2020 椎名深雪
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <mutex>
#include <akari/core/parallel.h>
#include <akari/kernel/integrators/cpu/integrator.h>
#include <akari/core/film.h>
#include <akari/core/logger.h>
#include <akari/kernel/scene.h>
#include <akari/kernel/interaction.h>
#include <akari/kernel/material.h>
#include <akari/kernel/sampling.h>
#include <akari/core/arena.h>
#include <akari/common/smallarena.h>
#include <akari/kernel/pathtracer.h>
#include <akari/core/progress.hpp>
namespace akari {
    namespace cpu {

        AKR_VARIANT void AmbientOcclusion<C>::render(const Scene<C> &scene, Film<C> *film) const {
            AKR_ASSERT_THROW(all(film->resolution() == scene.camera.resolution()));
            auto n_tiles = Point2i(film->resolution() + Point2i(tile_size - 1)) / Point2i(tile_size);
            auto Li = [=, &scene](Ray3f ray, Sampler<C> &sampler) -> Spectrum {
                (void)scene;
                Intersection<C> intersection;
                if (scene.intersect(ray, &intersection)) {
                    auto trig = scene.get_triangle(intersection.geom_id, intersection.prim_id);
                    Frame3f frame(trig.ng());
                    auto w = sampling<C>::cosine_hemisphere_sampling(sampler.next2d());
                    w = frame.local_to_world(w);
                    ray = Ray3f(trig.p(intersection.uv), w);
                    intersection = Intersection<C>();
                    if (scene.intersect(ray, &intersection) && intersection.t < occlude)
                        return Spectrum(0);
                    return Spectrum(1);
                    // return trig.ng() * 0.5 + 0.5;
                }
                return Spectrum(0);
                // debug("{}\n", ray.d);
                // return Spectrum(ray.d * 0.5f + 0.5f);
            };
            debug("resolution: {}, tile size: {}, tiles: {}", film->resolution(), tile_size, n_tiles);
            std::mutex mutex;

            parallel_for_2d(n_tiles, [=, &scene, &mutex](const Point2i &tile_pos, int tid) {
                (void)tid;
                Bounds2i tileBounds = Bounds2i{tile_pos * (int)tile_size, (tile_pos + Vector2i(1)) * (int)tile_size};
                auto boxed_tile = film->boxed_tile(tileBounds);
                auto &tile = *boxed_tile.get();
                auto &camera = scene.camera;
                auto sampler = scene.sampler;
                for (int y = tile.bounds.pmin.y(); y < tile.bounds.pmax.y(); y++) {
                    for (int x = tile.bounds.pmin.x(); x < tile.bounds.pmax.x(); x++) {
                        sampler.set_sample_index(x + y * film->resolution().x());
                        for (int s = 0; s < spp; s++) {
                            sampler.start_next_sample();
                            CameraSample<C> sample =
                                camera.generate_ray(sampler.next2d(), sampler.next2d(), Point2i(x, y));
                            auto L = Li(sample.ray, sampler);
                            tile.add_sample(Point2f(x, y), L, 1.0f);
                        }
                    }
                }
                std::lock_guard<std::mutex> _(mutex);
                film->merge_tile(tile);
            });
        }

        AKR_VARIANT void PathTracer<C>::render(const Scene<C> &scene, Film<C> *film) const {
            AKR_ASSERT_THROW(all(film->resolution() == scene.camera.resolution()));
            int max_depth = 5;
            auto n_tiles = Point2i(film->resolution() + Point2i(tile_size - 1)) / Point2i(tile_size);
            auto Li = [=, &scene](Ray3f ray, Sampler<C> &sampler, SmallArena *arena) -> Spectrum {
                GenericPathTracer<C> pt;
                pt.sampler = sampler;
                pt.max_depth = max_depth;
                while (true) {
                    auto hit = scene.intersect(ray);
                    if (!hit) {
                        pt.on_miss(scene, ray);
                        break;
                    }
                    SurfaceHit<C> surface_hit(ray, hit.value());
                    auto trig = scene.get_triangle(surface_hit.geom_id, surface_hit.prim_id);
                    surface_hit.material = trig.material;
                    SurfaceInteraction<C> si(surface_hit.uv, trig);

                    auto has_event = pt.on_surface_scatter(si, surface_hit);
                    if (!has_event) {
                        break;
                    }
                    astd::optional<DirectLighting<C>> has_direct =
                        pt.compute_direct_lighting(si, surface_hit, pt.select_light(scene));
                    if (has_direct) {
                        auto &direct = has_direct.value();
                        if (!direct.color.is_black() && !scene.occlude(direct.shadow_ray)) {
                            pt.L += direct.color;
                        }
                    }
                    auto event = has_event.value();
                    pt.beta *= event.beta;
                    pt.depth++;
                    ray = event.ray;
                }
                sampler = pt.sampler;
                return pt.L;
            };
            std::mutex mutex;
            auto num_threads = num_work_threads();
            auto _arena = MemoryArena<>(astd::pmr::polymorphic_allocator<>(active_device()->managed_resource()));
            std::vector<SmallArena> small_arenas;
            for (auto i = 0u; i < num_threads; i++) {
                size_t size = 256 * 1024;
                small_arenas.emplace_back(_arena.alloc_bytes(size), size);
            }
            auto reporter =
                std::make_shared<ProgressReporter>(n_tiles.x() * n_tiles.y(), [=](size_t cur, size_t total) {
                    bool show = false;
                    if (spp < 32) {
                        show = cur % 32 == 0;
                    } else {
                        show = true;
                    }

                    if (show)
                        show_progress(double(cur) / double(total), 60);

                    if (cur == total) {

                        putchar('\n');
                    }
                });
            parallel_for_2d(n_tiles, [=, &scene, &mutex, &small_arenas](const Point2i &tile_pos, int tid) {
                (void)tid;
                Bounds2i tileBounds = Bounds2i{tile_pos * (int)tile_size, (tile_pos + Vector2i(1)) * (int)tile_size};
                auto tile = film->tile(tileBounds);
                auto &camera = scene.camera;
                auto &arena = small_arenas[tid];
                auto sampler = scene.sampler;
                for (int y = tile.bounds.pmin.y(); y < tile.bounds.pmax.y(); y++) {
                    for (int x = tile.bounds.pmin.x(); x < tile.bounds.pmax.x(); x++) {
                        sampler.set_sample_index(x + y * film->resolution().x());
                        for (int s = 0; s < spp; s++) {
                            sampler.start_next_sample();
                            CameraSample<C> sample =
                                camera.generate_ray(sampler.next2d(), sampler.next2d(), Point2i(x, y));
                            auto L = Li(sample.ray, sampler, &arena);
                            tile.add_sample(Point2f(x, y), L, 1.0f);
                            arena.reset();
                        }
                    }
                }
                std::lock_guard<std::mutex> _(mutex);
                reporter->update();
                film->merge_tile(tile);
            });
        }
        AKR_RENDER_CLASS(AmbientOcclusion)
        AKR_RENDER_CLASS(PathTracer)
    } // namespace cpu
} // namespace akari
