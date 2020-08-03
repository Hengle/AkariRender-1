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

#include <akari/common/fwd.hpp>
#include <akari/common/math.hpp>
namespace akari {
    template <typename Float, int N> struct Color : Array<Float, N> {
        using Base = Array<Float, N>;
        using Base::Base;
        using value_t = Float;
        static constexpr size_t size = N;
        AKR_ARRAY_IMPORT(Base, Color)
    };

    template <typename Float> Color<Float, 3> linear_to_srgb(const Color<Float, 3> &L) {
        using Color3f = Color<Float, 3>;
        return select(L < 0.0031308, L * 12.92, 1.055 * pow(L, Color3f(2.4) - 0.055));
    }
    template <typename Float> Color<Float, 3> srgb_to_linear(const Color<Float, 3> &S) {
        using Color3f = Color<Float, 3>;
        return select(S < 0.04045, S / 12.92, pow((S + 0.055) / 1.055), Color3f(1.0 / 2.4));
    }
} // namespace akari
