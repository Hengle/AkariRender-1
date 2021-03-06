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
#include <vector_types.h>
#include <algorithm>
#include <execution>
#include <akari/core/parallel.h>
#include <akari/kernel/integrators/gpu/integrator.h>
#include <akari/core/film.h>
#include <akari/core/logger.h>
#include <akari/kernel/scene.h>
#include <akari/kernel/interaction.h>
#include <akari/kernel/material.h>
#include <akari/kernel/sampling.h>
#include <akari/core/arena.h>
#include <akari/common/smallarena.h>
#include <akari/kernel/cuda/launch.h>
#include <akari/kernel/pathtracer.h>
#include <akari/kernel/cuda/workqueue.h>
#include <akari/kernel/integrators/gpu/workitem-soa.h>
#include <akari/core/profiler.h>
#include <akari/core/progress.hpp>
namespace akari::gpu {

    AKR_VARIANT void AmbientOcclusion<C>::render(const Scene<C> &scene, Film<C> *film) const {
        if constexpr (std::is_same_v<Float, float>) {
            AKR_ASSERT_THROW(all(film->resolution() == scene.camera.resolution()));
            auto n_tiles = int2(film->resolution() + int2(tile_size - 1)) / int2(tile_size);
            auto Li = AKR_GPU_LAMBDA(Ray3f ray, Sampler<C> & sampler)->Spectrum {
                Intersection<C> intersection;
                if (scene.intersect(ray, &intersection)) {
                    auto trig = scene.get_triangle(intersection.geom_id, intersection.prim_id);
                    Frame3f frame(trig.ng());
                    auto w = sampling<C>::cosine_hemisphere_sampling(sampler.next2d());
                    w = frame.local_to_world(w);
                    ray = Ray3f(trig.p(intersection.uv), w);

                    if (auto isct = scene.intersect(ray)) {
                        if (isct.value().t < occlude)
                            return Spectrum(0);
                    }

                    return Spectrum(1);
                }
                return Spectrum(0);
            };
            debug("GPU RTAO resolution: {}, tile size: {}, tiles: {}", film->resolution(), tile_size, n_tiles);
            double gpu_time = 0;
            for (int tile_y = 0; tile_y < n_tiles.y; tile_y++) {
                for (int tile_x = 0; tile_x < n_tiles.x; tile_x++) {
                    int2 tile_pos(tile_x, tile_y);
                    Bounds2i tileBounds = Bounds2i{tile_pos * (int)tile_size, (tile_pos + int2(1)) * (int)tile_size};
                    auto boxed_tile = film->boxed_tile(tileBounds);
                    auto &camera = scene.camera;
                    auto sampler = scene.sampler;
                    auto extents = tileBounds.extents();
                    auto tile = boxed_tile.get();
                    auto resolution = film->resolution();
                    Timer timer;
                    launch(
                        "RTAO", extents.x * extents.y, AKR_GPU_LAMBDA(int tid) {
                            int tx = tid % extents.x;
                            int ty = tid / extents.x;
                            int x = tx + tileBounds.pmin.x;
                            int y = ty + tileBounds.pmin.y;
                            sampler.set_sample_index(x + y * resolution.x);
                            Spectrum acc;
                            for (int s = 0; s < spp; s++) {
                                sampler.start_next_sample();
                                CameraSample<C> sample =
                                    camera.generate_ray(sampler.next2d(), sampler.next2d(), int2(x, y));
                                auto L = Li(sample.ray, sampler);
                                acc += L;
                            }
                            tile->add_sample(float2(x, y), acc / spp, 1.0f);
                        });
                    CUDA_CHECK(cudaDeviceSynchronize());
                    gpu_time += timer.elapsed_seconds();
                    film->merge_tile(*tile);
                }
            }
            info("total gpu time {}s", gpu_time);
        } else {
            fatal("only float is supported for gpu");
        }
    }
    AKR_RENDER_CLASS(AmbientOcclusion)
    AKR_VARIANT struct PathTracerImpl {
        AKR_IMPORT_TYPES()
        using Allocator = astd::pmr::polymorphic_allocator<>;
        using RayQueue = WorkQueue<RayWorkItem<C>>;
        int spp;
        int tile_size = 512;
        int max_depth = 5;
        float ray_clamp;
        size_t MAX_QUEUE_SIZE;
        SOA<PathState<C>> path_states;
        RayQueue *ray_queue[2] = {nullptr, nullptr};
        using MaterialQueue = WorkQueue<MaterialWorkItem<C>>;
        using MaterialWorkQueues = astd::array<MaterialQueue *, Material<C>::num_types - 1>;
        using ShadowRayQueue = WorkQueue<ShadowRayWorkItem<C>>;
        MaterialWorkQueues material_queues;
        ShadowRayQueue *shadow_ray_queue;
        bool wavefront;
        PathTracerImpl(Allocator &allocator, const PathTracer<C> &pt)
            : spp(pt.spp), tile_size(pt.tile_size), max_depth(pt.max_depth), ray_clamp(pt.ray_clamp),
              wavefront(pt.wavefront) {
            MAX_QUEUE_SIZE = tile_size * tile_size;
            path_states = SOA<PathState<C>>(MAX_QUEUE_SIZE, allocator);
            ray_queue[0] = allocator.template new_object<RayQueue>(MAX_QUEUE_SIZE, allocator);
            // ray_queue[1] = allocator.template new_object<RayQueue>(MAX_QUEUE_SIZE, allocator);
            shadow_ray_queue = allocator.template new_object<ShadowRayQueue>(MAX_QUEUE_SIZE, allocator);
            for (size_t i = 0; i < material_queues.size(); i++) {
                material_queues[i] = allocator.template new_object<MaterialQueue>(MAX_QUEUE_SIZE, allocator);
            }
        }
        void render(const Scene<C> &scene, Film<C> *film);
    };
    AKR_VARIANT void PathTracer<C>::render(const Scene<C> &scene, Film<C> *film) const {
        if constexpr (std::is_same_v<Float, float>) {
            auto resource = TrackedManagedMemoryResource(active_device()->managed_resource());
            auto allocator = astd::pmr::polymorphic_allocator(&resource);
            PathTracerImpl<C> tracer(allocator, *this);
            tracer.render(scene, film);
        } else {
            fatal("only float is supported for gpu");
        }
    }
    struct ExtensionRay;
    struct EvaluateMaterial;
    struct ShadowRay;
    AKR_VARIANT void PathTracerImpl<C>::render(const Scene<C> &scene, Film<C> *film) {
        std::shared_ptr<ProgressReporter> reporter;
        auto n_tiles = int2(film->resolution() + int2(tile_size - 1)) / int2(tile_size);
        struct WorkTile {
            int2 tile_pos;
            Bounds2i tile_bounds;
            Box<Tile<C>> tile;
            WorkTile(int2 tile_pos, Bounds2i tile_bounds, Film<C> *film)
                : tile_pos(tile_pos), tile_bounds(tile_bounds), tile(film->boxed_tile(tile_bounds)) {}
        };

        std::list<WorkTile> tiles;
        for (int tile_y = 0; tile_y < n_tiles.y; tile_y++) {
            for (int tile_x = 0; tile_x < n_tiles.x; tile_x++) {
                int2 tile_pos(tile_x, tile_y);
                Bounds2i tileBounds = Bounds2i{tile_pos * (int)tile_size, (tile_pos + int2(1)) * (int)tile_size};
                tiles.emplace_back(tile_pos, tileBounds, film);
            }
        }
        reporter = std::make_shared<ProgressReporter>(tiles.size(), [](size_t cur, size_t total) {
            show_progress(double(cur) / double(total), 60);
            if (cur == total) {
                putchar('\n');
            }
        });
        auto render_tile_mega = [&](Tile<C> *tile, const int2 &tile_pos, const Bounds2i &tile_bounds) {
            auto &camera = scene.camera;
            auto sampler = scene.sampler;
            auto extents = tile_bounds.extents();
            auto resolution = film->resolution();
            launch(
                "Megakernel PT", extents.x * extents.y, AKR_GPU_LAMBDA(int tid) {
                    int tx = tid % extents.x;
                    int ty = tid / extents.x;
                    int x = tx + tile_bounds.pmin.x;
                    int y = ty + tile_bounds.pmin.y;
                    sampler.set_sample_index(x + y * resolution.x);
                    Spectrum acc;
                    for (int s = 0; s < spp; s++) {
                        sampler.start_next_sample();
                        GenericPathTracer<C> pt;
                        pt.depth = 0;
                        pt.max_depth = max_depth;
                        pt.sampler = sampler;
                        pt.run_megakernel(scene, camera, int2(x, y));
                        sampler = pt.sampler;
                        auto rad = pt.L.clamp_zero();
                        rad = min(rad, Spectrum(ray_clamp));
                        acc += rad;
                    }

                    tile->add_sample(float2(x, y), acc / spp, 1.0f);
                });
            cuLaunchHostFunc((cudaStream_t) nullptr, [](void *reporter) { ((ProgressReporter *)reporter)->update(); },
                             reporter.get());
        };
        auto render_tile_wavefront = [&](Tile<C> *tile, const int2 &tile_pos, const Bounds2i &tileBounds) {
            auto &camera = scene.camera;
            auto _sampler = scene.sampler;
            auto extents = tileBounds.extents();
            auto resolution = film->resolution();
            auto _spp = this->spp;
            auto get_pixel = AKR_GPU_LAMBDA(int pixel_id) {
                int tx = pixel_id % extents.x;
                int ty = pixel_id / extents.x;
                int x = tx + tileBounds.pmin.x;
                int y = ty + tileBounds.pmin.y;
                return int2(x, y);
            };
            auto launch_size = extents.x * extents.y;
            launch(
                "Set Path State", launch_size, AKR_GPU_LAMBDA(int tid) {
                    auto px = get_pixel(tid);
                    int x = px.x, y = px.y;
                    auto sampler = _sampler;
                    sampler.set_sample_index(x + y * resolution.x);
                    PathState<C> path_state = path_states[tid];
                    path_state.sampler = sampler;
                    path_state.L = Spectrum(0);
                    path_state.depth = 0;
                    path_state.iterations = 0;
                    path_state.state = PathKernelState::RayGen;
                    path_states[tid] = path_state;
                });
            for (int s = 0; s < _spp; s++) {
                for (int depth = 0; depth <= max_depth; depth++) {
                    launch(
                        "Ray Generation", launch_size, AKR_GPU_LAMBDA(int tid) {
                            auto px = get_pixel(tid);
                            int x = px.x, y = px.y;
                            PathState<C> path_state = path_states[tid];
                            if (path_state.state != PathKernelState::RayGen)
                                return;
                            path_state.state = PathKernelState::ExtensionRay;
                            path_state.L = Spectrum(0);
                            path_state.beta = Spectrum(1.0f);
                            path_state.sampler.start_next_sample();
                            path_state.depth = 0;
                            auto pt = path_state.path_tracer();
                            CameraSample<C> sample = pt.camera_ray(camera, px);
                            RayWorkItem<C> ray_item = (*ray_queue[0])[tid];
                            ray_item.pixel = tid;
                            ray_item.ray = sample.ray;
                            (*ray_queue[0])[tid] = ray_item;
                            path_state.update(pt);
                            path_states[tid] = path_state;
                        });

                    for (size_t i = 0; i < material_queues.size(); i++) {
                        launch_single(
                            "Reset Material Queue", AKR_GPU_LAMBDA() { material_queues[i]->clear(); });
                    }

                    launch<ExtensionRay>(
                        "Extension Ray", launch_size, AKR_GPU_LAMBDA(int tid) {
                            PathState<C> path_state = path_states[tid];
                            if (path_state.state != PathKernelState::ExtensionRay)
                                return;
                            RayWorkItem<C> ray_item = (*ray_queue[0])[tid];
                            path_state.state = PathKernelState::HitNothing;
                            Intersection<C> intersection;
                            if (scene.intersect(ray_item.ray, &intersection)) {
                                auto mat_idx =
                                    scene.meshes[intersection.geom_id].material_indices[intersection.prim_id];
                                if (mat_idx < 0) {
                                    path_states[tid] = path_state;
                                    return;
                                }
                                auto *material = scene.meshes[intersection.geom_id].materials[mat_idx];
                                if (!material) {
                                    path_states[tid] = path_state;
                                    return;
                                }
                                int pixel = ray_item.pixel;
                                AKR_ASSERT(pixel == tid);

                                float _u = path_state.sampler.next1d();
                                auto [mat, pdf] = material->select_material(_u, intersection.uv);
                                if (!mat) {
                                    path_states[tid] = path_state;
                                    return;
                                }
                                MaterialWorkItem<C> material_item;
                                material_item.pdf = pdf;
                                material_item.pixel = pixel;
                                material_item.material = mat;
                                material_item.geom_id = intersection.geom_id;
                                material_item.prim_id = intersection.prim_id;
                                material_item.uv = intersection.uv;
                                material_item.wo = -ray_item.ray.d;
                                auto queue_idx = mat->typeindex();
                                AKR_ASSERT(queue_idx != Material<C>::template indexof<MixMaterial<C>>());
                                AKR_ASSERT(queue_idx >= 0 && queue_idx < material_queues.size());
                                path_state.state = PathKernelState::EvalMaterial;
                                material_queues[queue_idx]->append(material_item);
                            }
                            path_states[tid] = path_state;
                        });
                    launch(
                        "Hit Nothing", launch_size, AKR_GPU_LAMBDA(int tid) {
                            PathState<C> path_state = path_states[tid];
                            if (path_state.state != PathKernelState::HitNothing) {
                                return;
                            }
                            path_state.state = PathKernelState::Splat;
                            path_states[tid] = path_state;
                        });
                    for (auto material_queue : material_queues) {
                        launch<EvaluateMaterial>(
                            "Evaluate Material", launch_size, AKR_GPU_LAMBDA(int tid) {
                                if (tid >= material_queue->elements_in_queue())
                                    return;
                                MaterialWorkItem<C> material_item = (*material_queue)[tid];
                                int pixel = material_item.pixel;
                                PathState<C> path_state = path_states[pixel];
                                AKR_ASSERT(path_state.state == PathKernelState::EvalMaterial);
                                path_state.state = PathKernelState::EvalMaterial;
                                auto pt = path_state.path_tracer();
                                pt.depth = path_state.depth;
                                pt.max_depth = max_depth;
                                path_state.depth++;
                                auto surface_hit = material_item.surface_hit();
                                auto trig = scene.get_triangle(material_item.geom_id, material_item.prim_id);
                                SurfaceInteraction<C> si(surface_hit.uv, trig);

                                auto has_event = pt.on_surface_scatter(si, surface_hit, material_item.pdf);
                                if (has_event) {
                                    auto event = has_event.value();

                                    // Direct Light Sampling
                                    astd::optional<DirectLighting<C>> has_direct =
                                        pt.compute_direct_lighting(si, surface_hit, pt.select_light(scene));
                                    path_state.state = PathKernelState::ExtensionRay;
                                    if (has_direct) {
                                        auto direct = has_direct.value();
                                        if (!direct.color.is_black()) {
                                            ShadowRayWorkItem<C> shadow_ray_item(direct);
                                            shadow_ray_item.pixel = pixel;
                                            (*shadow_ray_queue)[pixel] = (shadow_ray_item);
                                            path_state.state = PathKernelState::ShadowRay;
                                        }
                                    }
                                    pt.beta *= event.beta;
                                    RayWorkItem<C> ray_item;
                                    ray_item.pixel = pixel;
                                    ray_item.ray = event.ray;
                                    (*ray_queue[0])[pixel] = ray_item;

                                } else {
                                    path_state.state = PathKernelState::Splat;
                                }
                                path_state.update(pt);
                                path_states[pixel] = path_state;
                            });
                    }
                    launch<ShadowRay>(
                        "Shadow Ray", launch_size, AKR_GPU_LAMBDA(int tid) {
                            if (tid >= launch_size)
                                return;
                            PathState<C> path_state = path_states[tid];
                            if (path_state.state != PathKernelState::ShadowRay)
                                return;
                            ShadowRayWorkItem<C> shadow_ray_item = (*shadow_ray_queue)[tid];
                            DirectLighting<C> direct = shadow_ray_item.direct_lighting();
                            int pixel = shadow_ray_item.pixel;
                            if (pixel != tid) {
                                printf("%d %d\n", pixel, tid);
                            }
                            AKR_ASSERT(pixel == tid);
                            if (!scene.occlude(direct.shadow_ray)) {
                                path_state.L += direct.color;
                            }
                            path_state.state = PathKernelState::ExtensionRay;
                            path_states[tid] = path_state;
                        });
                    launch(
                        "Splat", launch_size, AKR_GPU_LAMBDA(int tid) {
                            PathState<C> path_state = path_states[tid];
                            if (path_state.state != PathKernelState::Splat) {
                                return;
                            }
                            path_state.iterations++;
                            if (path_state.iterations == _spp) {
                                path_state.state = PathKernelState::Completed;
                            } else
                                path_state.state = PathKernelState::RayGen;
                            auto p = get_pixel(tid);
                            auto rad = path_state.L.clamp_zero();
                            rad = min(rad, Spectrum(ray_clamp));
                            tile->add_sample(float2(p), rad, 1.0f);
                            path_states[tid] = path_state;
                        });
                }
            }
            cuLaunchHostFunc((cudaStream_t) nullptr, [](void *reporter) { ((ProgressReporter *)reporter)->update(); },
                             reporter.get());
        };

        debug("tile size:{} n_tiles: {}", tile_size, n_tiles);
        if (wavefront) {
            for (auto &item : tiles) {
                auto &[tile_pos, tile_bounds, tile] = item;
                render_tile_wavefront(tile.get(), tile_pos, tile_bounds);
            }
        } else {
            for (auto &item : tiles) {
                auto &[tile_pos, tile_bounds, tile] = item;
                render_tile_mega(tile.get(), tile_pos, tile_bounds);
            }
        }
        active_device()->sync();
        std::for_each(std::execution::par_unseq, tiles.begin(), tiles.end(),
                      [=](const WorkTile &item) { film->merge_tile(*item.tile.get()); });
        print_kernel_stats();
    }
    AKR_RENDER_CLASS(PathTracer)

} // namespace akari::gpu