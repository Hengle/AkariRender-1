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
#include <akari/kernel/material.h>
#include <akari/kernel/shape.h>
namespace akari {
    AKR_VARIANT class BSDF;
    AKR_VARIANT struct Intersection;
    AKR_VARIANT struct SurfaceInteraction {
        AKR_IMPORT_TYPES()
        Triangle<C> triangle;
        Float3 p;
        BSDF<C> bsdf;
        Float3 ng, ns;
        float2 texcoords;

        AKR_XPU SurfaceInteraction(const Intersection<C> &isct, const Triangle<C> &triangle)
            : triangle(triangle), p(isct.p), ng(isct.ng), ns(triangle.ns(isct.uv)),
              texcoords(triangle.texcoord(isct.uv)) {}
        AKR_XPU SurfaceInteraction(const float2 &uv, const Triangle<C> &triangle)
            : triangle(triangle), p(triangle.p(uv)), ng(triangle.ng()), ns(triangle.ns(uv)),
              texcoords(triangle.texcoord(uv)) {}
    };
} // namespace akari