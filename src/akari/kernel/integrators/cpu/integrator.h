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
#include <akari/common/taggedpointer.h>
namespace akari {
    struct AOV {
        static constexpr uint32_t color = 0;
        static constexpr uint32_t albedo = 1;
        static constexpr uint32_t depth = 2;
        static constexpr uint32_t normal = 3;
    };
    namespace cpu {
        AKR_VARIANT class AmbientOcclusion {
          public:
            int spp = 16;
            int tile_size = 16;
            AKR_IMPORT_TYPES()
            void render(const Scene<C> &scene, Film<C> *out) const;
        };
        AKR_VARIANT class Integrator : public TaggedPointer<AmbientOcclusion<C>> {
          public:
            AKR_IMPORT_TYPES()
            using TaggedPointer<AmbientOcclusion<C>>::TaggedPointer;
            void render(const Scene<C> &scene, Film<C> *out) const { AKR_TAGGED_DISPATCH(render, scene, out); }
        };
    } // namespace cpu
} // namespace akari