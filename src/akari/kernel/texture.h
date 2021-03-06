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
#include <akari/common/color.h>
#include <akari/core/image.hpp>

namespace akari {
    AKR_VARIANT class ConstantTexture {
      public:
        AKR_IMPORT_TYPES()
        ConstantTexture(Spectrum v) : value(v) {}
        Spectrum value;
        AKR_XPU Spectrum evaluate(const float2 &texcoords) const { return value; }
        Float integral() const { return luminance(value); }
    };

    AKR_VARIANT class ImageTexture {
      public:
        AKR_IMPORT_TYPES()
        RGBAImage::View image;
        ImageTexture() = default;
        AKR_XPU ImageTexture(RGBAImage::View image) : image(image) {}
        AKR_XPU Spectrum evaluate(const float2 &texcoords) const {
            float2 tc = fmod(texcoords, Array2f(1.0f));
            tc.y = 1.0f - tc.y;
            return image(tc).rgb;
        }
        Float integral() const {
            Float I = 0;
            for (size_t i = 0; i < image.resolution().x * image.resolution().y; i++) {
                I += luminance(image.data()[i].rgb);
            }
            return I / (image.resolution().x * image.resolution().y);
        }
    };
    AKR_VARIANT class Texture : public Variant<ConstantTexture<C>, ImageTexture<C>> {
      public:
        AKR_IMPORT_TYPES()
        using Variant<ConstantTexture<C>, ImageTexture<C>>::Variant;
        AKR_XPU Spectrum evaluate(const float2 &texcoords) const { AKR_VAR_DISPATCH(evaluate, texcoords); }
        Float integral() const {
            return dispatch_cpu([=](auto &&arg) { return arg.integral(); });
        }
    };
} // namespace akari