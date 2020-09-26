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

#include <akari/common/variant.h>
#include <akari/common/math.h>
#include <akari/kernel/bsdf-funcs.h>
#include <akari/kernel/microfacet.h>
#include <akari/kernel/texture.h>
#include <akari/kernel/sampler.h>
#include <akari/kernel/interaction.h>

namespace akari {
    enum BSDFType : int {
        BSDF_NONE = 0u,
        BSDF_REFLECTION = 1u << 0u,
        BSDF_TRANSMISSION = 1u << 1u,
        BSDF_DIFFUSE = 1u << 2u,
        BSDF_GLOSSY = 1u << 3u,
        // BSDF_SPECULAR = 1u << 4u,
        BSDF_ALL = BSDF_DIFFUSE | BSDF_GLOSSY | BSDF_REFLECTION | BSDF_TRANSMISSION,
    };

    AKR_VARIANT struct BSDFSample {
        AKR_IMPORT_TYPES()
        Vector3f wi = Vector3f(0);
        Float pdf = 0.0;
        Spectrum f = Spectrum(0.0f);
        BSDFType sampled = BSDF_NONE;
    };
    AKR_VARIANT struct BSDFSampleContext {
        AKR_IMPORT_TYPES()
        const Point2f u1;
        const Vector3f wo;
        AKR_XPU BSDFSampleContext(const Point2f &u1, const Vector3f &wo) : u1(u1), wo(wo) {}
    };
    AKR_VARIANT class DiffuseBSDF {
        AKR_IMPORT_TYPES();
        Spectrum R;

      public:
        AKR_XPU DiffuseBSDF(const Spectrum &R) : R(R) {}
        [[nodiscard]] AKR_XPU Float evaluate_pdf(const Vector3f &wo, const Vector3f &wi) const {
            if (bsdf<C>::same_hemisphere(wo, wi)) {
                return sampling<C>::cosine_hemisphere_pdf(std::abs(bsdf<C>::cos_theta(wi)));
            }
            return 0.0f;
        }
        [[nodiscard]] AKR_XPU Spectrum evaluate(const Vector3f &wo, const Vector3f &wi) const {
            if (bsdf<C>::same_hemisphere(wo, wi)) {
                return R * Constants<Float>::InvPi();
            }
            return Spectrum(0.0f);
        }
        [[nodiscard]] AKR_XPU BSDFType type() const { return BSDFType(BSDF_DIFFUSE | BSDF_REFLECTION); }
        AKR_XPU Spectrum sample(const Point2f &u, const Vector3f &wo, Vector3f *wi, Float *pdf,
                                BSDFType *sampledType) const {
            *wi = sampling<C>::cosine_hemisphere_sampling(u);
            if (!bsdf<C>::same_hemisphere(wo, *wi)) {
                wi->y() = -wi->y();
            }
            *sampledType = type();
            *pdf = sampling<C>::cosine_hemisphere_pdf(std::abs(bsdf<C>::cos_theta(*wi)));
            return R * Constants<Float>::InvPi();
        }
    };

    AKR_VARIANT class BSDFClosure : Variant<DiffuseBSDF<C>> {
      public:
        using Variant<DiffuseBSDF<C>>::Variant;
        AKR_IMPORT_TYPES();
        [[nodiscard]] AKR_XPU Float evaluate_pdf(const Vector3f &wo, const Vector3f &wi) const {
            AKR_VAR_DISPATCH(evaluate_pdf, wo, wi);
        }
        [[nodiscard]] AKR_XPU Spectrum evaluate(const Vector3f &wo, const Vector3f &wi) const {
            AKR_VAR_DISPATCH(evaluate, wo, wi);
        }
        [[nodiscard]] AKR_XPU BSDFType type() const { AKR_VAR_DISPATCH(type); }
        [[nodiscard]] AKR_XPU bool match_flags(BSDFType flag) const { return ((uint32_t)type() & (uint32_t)flag) != 0; }
        AKR_XPU Spectrum sample(const Point2f &u, const Vector3f &wo, Vector3f *wi, Float *pdf,
                                BSDFType *sampledType) const {
            AKR_VAR_DISPATCH(sample, u, wo, wi, pdf, sampledType);
        }
    };

    AKR_VARIANT class BSDF {
        AKR_IMPORT_TYPES()
        BSDFClosure<C> closure_;
        Normal3f ng, ns;
        Frame3f frame;
        Float choice_pdf = 1.0f;

      public:
        BSDF() = default;
        AKR_XPU explicit BSDF(const Normal3f &ng, const Normal3f &ns) : ng(ng), ns(ns) { frame = Frame3f(ns); }
        AKR_XPU bool null() const { return closure().null(); }
        AKR_XPU void set_closure(const BSDFClosure<C> &closure) { closure_ = closure; }
        AKR_XPU void set_choice_pdf(Float pdf) { choice_pdf = pdf; }
        AKR_XPU const BSDFClosure<C> &closure() const { return closure_; }
        [[nodiscard]] AKR_XPU Float evaluate_pdf(const Vector3f &wo, const Vector3f &wi) const {
            auto pdf = closure().evaluate_pdf(frame.world_to_local(wo), frame.world_to_local(wi));
            return pdf * choice_pdf;
        }
        [[nodiscard]] AKR_XPU Spectrum evaluate(const Vector3f &wo, const Vector3f &wi) const {
            auto f = closure().evaluate(frame.world_to_local(wo), frame.world_to_local(wi));
            return f;
        }

        [[nodiscard]] AKR_XPU BSDFType type() const { return closure().type(); }
        [[nodiscard]] AKR_XPU bool match_flags(BSDFType flag) const { return closure().match_flags(flag); }
        AKR_XPU BSDFSample<C> sample(const BSDFSampleContext<C> &ctx) const {
            auto wo = frame.world_to_local(ctx.wo);
            Vector3f wi;
            BSDFSample<C> sample;
            sample.f = closure().sample(ctx.u1, wo, &wi, &sample.pdf, &sample.sampled);
            sample.wi = frame.local_to_world(wi);
            sample.pdf *= choice_pdf;
            return sample;
        }
    };
    AKR_VARIANT struct MaterialEvalContext {
        AKR_IMPORT_TYPES()
        Point2f u1, u2;
        Point2f texcoords;
        Normal3f ng, ns;
        MaterialEvalContext() = default;
        AKR_XPU MaterialEvalContext(Sampler<C> sampler, const SurfaceInteraction<C> &si)
            : MaterialEvalContext(sampler, si.texcoords, si.ng, si.ns) {}
        AKR_XPU MaterialEvalContext(Sampler<C> sampler, const Point2f &texcoords, const Normal3f &ng,
                                    const Normal3f &ns)
            : u1(sampler.next2d()), u2(sampler.next2d()), texcoords(texcoords), ng(ng), ns(ns) {}
    };

    AKR_VARIANT class DiffuseMaterial {
      public:
        DiffuseMaterial(Texture<C> *color) : color(color) {}
        AKR_IMPORT_TYPES()
        Texture<C> *color;
        AKR_XPU BSDF<C> get_bsdf(MaterialEvalContext<C> &ctx) const {
            auto R = color->evaluate(ctx.texcoords);
            BSDF<C> bsdf(ctx.ng, ctx.ns);
            bsdf.set_closure((DiffuseBSDF<C>(R)));
            return bsdf;
        }
    };
    AKR_VARIANT class GlossyMaterial {
      public:
        AKR_IMPORT_TYPES()
        const Texture<C> *roughness = nullptr;
        const Texture<C> *color = nullptr;
    };
    AKR_VARIANT class EmissiveMaterial {
      public:
        AKR_IMPORT_TYPES()
        const Texture<C> *color;
        bool double_sided = false;
        AKR_XPU EmissiveMaterial(const Texture<C> *color, bool double_sided = false)
            : color(color), double_sided(double_sided) {}
    };
    AKR_VARIANT class MixMaterial;
    AKR_VARIANT class MixMaterial {
      public:
        AKR_IMPORT_TYPES()
        const Texture<C> *fraction;
        const Material<C> *material_A, *material_B;
    };
    AKR_VARIANT class Material : public Variant<DiffuseMaterial<C>, MixMaterial<C>, EmissiveMaterial<C>> {
      public:
        AKR_IMPORT_TYPES()
        using Variant<DiffuseMaterial<C>, MixMaterial<C>, EmissiveMaterial<C>>::Variant;

      private:
        AKR_XPU const Material<C> *select_material(Float &u, const Point2f &texcoords, Float *choice_pdf) const {
            if (this->template isa<EmissiveMaterial<C>>()) {
                return nullptr;
            }
            *choice_pdf = 1.0f;
            auto ptr = this;
            while (ptr->template isa<MixMaterial<C>>()) {
                auto frac = ptr->template get<MixMaterial<C>>()->fraction->evaluate(texcoords).x();
                if (u < frac) {
                    u = u / frac;
                    ptr = ptr->template get<MixMaterial<C>>()->material_A;
                    *choice_pdf *= 1.0f / frac;
                } else {
                    u = (u - frac) / (1.0f - frac);
                    ptr = ptr->template get<MixMaterial<C>>()->material_B;
                    *choice_pdf *= 1.0f / (1.0f - frac);
                }
            }
            return ptr;
        }
        AKR_XPU BSDF<C> get_bsdf0(MaterialEvalContext<C> &ctx) const {
            return this->dispatch([&](auto &&arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, MixMaterial<C>> || std::is_same_v<T, EmissiveMaterial<C>>) {
                    return BSDF<C>();
                } else
                    return arg.get_bsdf(ctx);
            });
        }

      public:
        AKR_XPU BSDF<C> get_bsdf(MaterialEvalContext<C> &ctx) const {
            Float choice_pdf = 0.0f;
            auto mat = select_material(ctx.u1[0], ctx.texcoords, &choice_pdf);
            auto bsdf = mat->get_bsdf0(ctx);
            bsdf.set_choice_pdf(choice_pdf);
            return bsdf;
        }
    };

} // namespace akari