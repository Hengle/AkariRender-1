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

#include <akari/core/akari.h>
#include <akari/common/fwd.h>
#include <akari/core/buffer.h>
#include <akari/core/arena.h>
#include <akari/kernel/meshview.h>
#include <akari/kernel/integrators/cpu/integrator.h>
// #include <akari/kernel/material.h>
namespace pybind11 {
    class module;
}
namespace akari {
    namespace py = pybind11;
    AKR_VARIANT class SceneGraphNode {
      public:
        using Float = typename C::Float;
        AKR_IMPORT_CORE_TYPES()
        virtual void commit() {}
        virtual ~SceneGraphNode() = default;
    };
    AKR_VARIANT class SceneNode;

    AKR_VARIANT class FilmNode : public SceneGraphNode<C> { public: };
    AKR_VARIANT struct RegisterSceneGraph { static void register_scene_graph(py::module &parent); };

} // namespace akari