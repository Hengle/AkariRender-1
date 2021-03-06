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

#pragma once
#include <akari/common/math.h>
#include <akari/common/distribution.h>
#include <akari/kernel/instance.h>
#include <akari/kernel/camera.h>
#include <akari/kernel/sampler.h>
#include <akari/kernel/shape.h>
#include <akari/kernel/light.h>
#ifdef AKR_ENABLE_EMBREE
#    include <akari/kernel/embree.inl>
#endif

namespace akari {
    AKR_VARIANT
    class EmbreeAccelerator;
    AKR_VARIANT
    class BVHAccelerator;
    AKR_VARIANT struct Intersection {
        AKR_IMPORT_TYPES()
        Float t = Constants<Float>::Inf();
        Float3 ng;
        float2 uv;
        int geom_id = -1;
        int prim_id = -1;
        bool is_instance = false;
        bool hit() const { return geom_id != -1 && prim_id != -1; }
    };
    AKR_VARIANT class Scene {
      public:
        AKR_IMPORT_TYPES()
        BufferView<MeshInstance<C>> meshes;
        Camera<C> camera;
        Sampler<C> sampler;
        BufferView<AreaLight<C>> area_lights;
        Variant<EmbreeAccelerator<C> *, BVHAccelerator<C> *> accel;
        Distribution1D<C> *light_distribution;
        AKR_XPU bool intersect(const Ray3f &ray, Intersection<C> *isct) const;
        AKR_XPU astd::optional<Intersection<C>> intersect(const Ray3f &ray) const {
            Intersection<C> isct;
            if (intersect(ray, &isct)) {
                return isct;
            }
            return astd::nullopt;
        }
        AKR_XPU bool occlude(const Ray3f &ray) const;

        void commit();
        AKR_XPU Triangle<C> get_triangle(int mesh_id, int prim_id) const {
            auto &mesh = meshes[mesh_id];
            Triangle<C> trig = akari::get_triangle<C>(mesh, prim_id);
            auto mat_idx = mesh.material_indices[prim_id];
            if (mat_idx != -1) {
                trig.material = mesh.materials[mat_idx];
            }
            return trig;
        }
        AKR_XPU astd::pair<const AreaLight<C> *, Float> select_light(const float2 &u) const {
            if (area_lights.size() == 0) {
                return {nullptr, Float(0.0f)};
            }
            Float pdf;
            size_t idx = light_distribution->sample_discrete(u[0], &pdf);
            // size_t idx = area_lights.size() * u[0];
            if (idx == area_lights.size()) {
                idx -= 1;
            }
            return {&area_lights[idx], pdf};
        }
    };
} // namespace akari
