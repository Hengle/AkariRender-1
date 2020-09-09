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
namespace akari {
    AKR_VARIANT struct bsdf {
        AKR_IMPORT_TYPES()
        static inline Float cos_theta(const Vector3f &w) { return w.y(); }

        static inline Float abs_cos_theta(const Vector3f &w) { return std::abs(cos_theta(w)); }

        static inline Float cos2_theta(const Vector3f &w) { return w.y() * w.y(); }

        static inline Float sin2_theta(const Vector3f &w) { return 1 - cos2_theta(w); }

        static inline Float sin_theta(const Vector3f &w) { return std::sqrt(std::fmax(0.0f, sin2_theta(w))); }

        static inline Float tan2_theta(const Vector3f &w) { return sin2_theta(w) / cos2_theta(w); }

        static inline Float tan_theta(const Vector3f &w) { return std::sqrt(std::fmax(0.0f, tan2_theta(w))); }

        static inline Float cos_phi(const Vector3f &w) {
            Float sinTheta = sin_theta(w);
            return (sinTheta == 0) ? 1 : std::clamp<Float>(w.x / sinTheta, -1, 1);
        }
        static inline Float sin_phi(const Vector3f &w) {
            Float sinTheta = sin_theta(w);
            return (sinTheta == 0) ? 0 : std::clamp<Float>(w.z / sinTheta, -1, 1);
        }

        static inline Float cos2_phi(const Vector3f &w) { return cos_phi(w) * cos_phi(w); }
        static inline Float sin2_phi(const Vector3f &w) { return sin_phi(w) * sin_phi(w); }

        static inline bool same_hemisphere(const Vector3f &wo, const Vector3f &wi) { return wo.y() * wi.y() >= 0; }

        static inline Vector3f reflect(const Vector3f &w, const Normal3f &n) {
            return -1.0f * w + 2.0f * dot(w, n) * n;
        }

        static inline bool refract(const Vector3f &wi, const Normal3f &n, Float eta, Vector3f *wt) {
            Float cosThetaI = dot(n, wi);
            Float sin2ThetaI = std::fmax(0.0f, 1.0f - cosThetaI * cosThetaI);
            Float sin2ThetaT = eta * eta * sin2ThetaI;
            if (sin2ThetaT >= 1)
                return false;

            Float cosThetaT = std::sqrt(1 - sin2ThetaT);

            *wt = eta * -wi + (eta * cosThetaI - cosThetaT) * n;
            return true;
        }
    };
} // namespace akari