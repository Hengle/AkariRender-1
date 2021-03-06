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
#include <akari/common/fwd.h>
#ifdef AKR_ENABLE_EMBREE
#    ifdef _MSC_VER
#        pragma warning(disable : 4324)

#    endif
#    include <embree3/rtcore.h>

namespace akari {
    AKR_VARIANT struct Intersection;
    AKR_VARIANT
    class EmbreeAccelerator {
        RTCScene rtcScene = nullptr;
        RTCDevice device = nullptr;

      public:
        using Float = typename C::Float;
        AKR_IMPORT_CORE_TYPES()
        EmbreeAccelerator() { device = rtcNewDevice(nullptr); }
        void build(Scene<C> &scene);
        bool intersect(const Ray<C> &ray, Intersection<C> *isct) const;
        bool occlude(const Ray<C> &ray) const;
        ~EmbreeAccelerator() {
            if (rtcScene)
                rtcReleaseScene(rtcScene);
            rtcReleaseDevice(device);
        }
    };
} // namespace akari
#else
namespace akari {
    AKR_VARIANT class EmbreeAccelerator {};
} // namespace akari
#endif