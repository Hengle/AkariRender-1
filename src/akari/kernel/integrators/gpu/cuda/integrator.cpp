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
namespace akari::gpu {
    AKR_VARIANT void AmbientOcclusion<C>::render(const Scene<C> &scene, Film<C> *film) const {
        if constexpr (std::is_same_v<Float, float>) {
            AKR_ASSERT_THROW(all(film->resolution() == scene.camera.resolution()));
            auto n_tiles = Point2i(film->resolution() + Point2i(tile_size - 1)) / Point2i(tile_size);
            auto Li = AKR_GPU_LAMBDA(Ray3f ray, Sampler<C> & sampler)->Spectrum {
                Intersection<C> intersection;
                if (scene.intersect(ray, &intersection)) {
                    Frame3f frame(intersection.ng);
                    auto w = sampling<C>::cosine_hemisphere_sampling(sampler.next2d());
                    w = frame.local_to_world(w);
                    ray = Ray3f(intersection.p, w);
                    intersection = Intersection<C>();
                    if (scene.intersect(ray, &intersection))
                        return Spectrum(0);
                    return Spectrum(1);
                }
                return Spectrum(0);
            };
            debug("resolution: {}, tile size: {}, tiles: {}\n", film->resolution(), tile_size, n_tiles);
            launch(
                "test", n_tiles.x() * n_tiles.y(), AKR_GPU_LAMBDA(int tid) {
                    Point2i tile_pos(tid % n_tiles.x(), tid / n_tiles.x());
                    Bounds2i tileBounds =
                        Bounds2i{tile_pos * (int)tile_size, (tile_pos + Vector2i(1)) * (int)tile_size};
                    // auto tile = film->tile(tileBounds);
                    auto &camera = scene.camera;
                    auto sampler = scene.sampler;
                    // for (int y = tile.bounds.pmin.y(); y < tile.bounds.pmax.y(); y++) {
                    //     for (int x = tile.bounds.pmin.x(); x < tile.bounds.pmax.x(); x++) {
                        int x = 0;
                        int y = 0;
                            sampler.set_sample_index(x + y * film->resolution().x());
                            for (int s = 0; s < spp; s++) {
                                sampler.start_next_sample();
                                CameraSample<C> sample;
                                camera.generate_ray(sampler.next2d(), sampler.next2d(), Point2i(x, y), &sample);
                                auto L = Li(sample.ray, sampler);
                                // tile.add_sample(Point2f(x, y), L, 1.0f);
                            }
                        //}
                    //}
                });
            cudaDeviceSynchronize();
        } else {
            fatal("only float is supported for gpu\n");
        }
    }
    AKR_RENDER_CLASS(AmbientOcclusion)
} // namespace akari::gpu