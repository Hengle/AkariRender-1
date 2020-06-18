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

#ifndef AKARIRENDER_BSDF_H
#define AKARIRENDER_BSDF_H

#include "interaction.h"
#include <akari/core/sampling.hpp>
#include <akari/core/spectrum.h>
#include <akari/render/geometry.hpp>
namespace akari {
    enum BSDFType : int {
        BSDF_NONE = 0u,
        BSDF_REFLECTION = 1u << 0u,
        BSDF_TRANSMISSION = 1u << 1u,
        BSDF_DIFFUSE = 1u << 2u,
        BSDF_GLOSSY = 1u << 3u,
        BSDF_SPECULAR = 1u << 4u,
        BSDF_ALL = BSDF_DIFFUSE | BSDF_GLOSSY | BSDF_SPECULAR | BSDF_REFLECTION | BSDF_TRANSMISSION,
    };

    inline Float cos_theta(const vec3 &w) { return w.y; }

    inline Float abs_cos_theta(const vec3 &w) { return std::abs(cos_theta(w)); }

    inline Float cos2_theta(const vec3 &w) { return w.y * w.y; }

    inline Float sin2_theta(const vec3 &w) { return 1 - cos2_theta(w); }

    inline Float sin_theta(const vec3 &w) { return std::sqrt(std::fmax(0.0f, sin2_theta(w))); }

    inline Float tan2_theta(const vec3 &w) { return sin2_theta(w) / cos2_theta(w); }

    inline Float tan_theta(const vec3 &w) { return std::sqrt(std::fmax(0.0f, tan2_theta(w))); }

    inline Float cos_phi(const vec3 &w) {
        Float sinTheta = sin_theta(w);
        return (sinTheta == 0) ? 1 : std::clamp<float>(w.x / sinTheta, -1, 1);
    }
    inline Float sin_phi(const vec3 &w) {
        Float sinTheta = sin_theta(w);
        return (sinTheta == 0) ? 0 : std::clamp<float>(w.z / sinTheta, -1, 1);
    }

    inline Float cos2_phi(const vec3 &w) { return cos_phi(w) * cos_phi(w); }
    inline Float sin2_phi(const vec3 &w) { return sin_phi(w) * sin_phi(w); }

    inline bool same_hemisphere(const vec3 &wo, const vec3 &wi) { return wo.y * wi.y >= 0; }

    inline vec3 reflect(const vec3 &w, const vec3 &n) { return -1.0f * w + 2.0f * dot(w, n) * n; }

    inline bool refract(const vec3 &wi, const vec3 &n, Float eta, vec3 *wt) {
        Float cosThetaI = dot(n, wi);
        Float sin2ThetaI = std::fmax(0.0f, 1.0f - cosThetaI * cosThetaI);
        Float sin2ThetaT = eta * eta * sin2ThetaI;
        if (sin2ThetaT >= 1)
            return false;

        Float cosThetaT = std::sqrt(1 - sin2ThetaT);

        *wt = eta * -wi + (eta * cosThetaI - cosThetaT) * n;
        return true;
    }

    struct BSDFSample {
        const vec3 wo;
        Float u0{};
        vec2 u{};
        vec3 wi{};
        Spectrum f{};
        Float pdf = 0;
        BSDFType sampledType = BSDF_NONE;
        inline BSDFSample(Float u0, const vec2 &u, const SurfaceInteraction &si);
    };

    class BSDFComponent {
      public:
        explicit BSDFComponent(BSDFType type) : type(type) {}
        const BSDFType type;
        [[nodiscard]] virtual Float evaluate_pdf(const vec3 &wo, const vec3 &wi) const {
            return abs_cos_theta(wi) * InvPi;
        }
        [[nodiscard]] virtual Spectrum evaluate(const vec3 &wo, const vec3 &wi) const = 0;
        virtual Spectrum sample(const vec2 &u, const vec3 &wo, vec3 *wi, Float *pdf, BSDFType *sampledType) const {
            *wi = cosine_hemisphere_sampling(u);
            if (!same_hemisphere(*wi, wo)) {
                wi->y *= -1;
            }
            *pdf = abs_cos_theta(*wi) * InvPi;
            *sampledType = type;
            return evaluate(wo, *wi);
        }
        [[nodiscard]] bool is_delta() const { return ((uint32_t)type & (uint32_t)BSDF_SPECULAR) != 0; }
        [[nodiscard]] bool match_flags(BSDFType flag) const { return ((uint32_t)type & (uint32_t)flag) != 0; }
        [[nodiscard]] virtual Float importance(const vec3 &wo) const { return 1.0f; }
    };
    class AKR_EXPORT BSDF {
        constexpr static int MaxBSDF = 8;
        std::array<const BSDFComponent *, MaxBSDF> components{};
        int nComp = 0;
        const Frame3f frame;
        vec3 Ng;
        vec3 Ns;
        void evaluate_importance(const vec3 &wo,std::array<Float, MaxBSDF> & func)const;
      public:
        BSDF(const vec3 &Ng, const vec3 &Ns) : frame(Ns), Ng(Ng), Ns(Ns) {}
        explicit BSDF(const SurfaceInteraction &si) : frame(si.Ns), Ng(si.Ng), Ns(si.Ns) {}
        void add_component(const BSDFComponent *comp) { components[nComp++] = comp; }
        [[nodiscard]] Float evaluate_pdf(const vec3 &woW, const vec3 &wiW) const;
        [[nodiscard]] vec3 local_to_world(const vec3 &w) const { return frame.local_to_world(w); }
        [[nodiscard]] vec3 world_to_local(const vec3 &w) const { return frame.world_to_local(w); }
        [[nodiscard]] Spectrum evaluate(const vec3 &woW, const vec3 &wiW) const;
        void sample(BSDFSample &sample) const;
        
    };
    inline BSDFSample::BSDFSample(Float u0, const vec2 &u, const SurfaceInteraction &si) : wo(si.wo), u0(u0), u(u) {}

} // namespace akari
#endif // AKARIRENDER_BSDF_H
