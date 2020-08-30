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
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <akari/core/nodes/scene.h>
#include <akari/kernel/embree.inl>
#include <akari/core/film.h>

namespace akari {
    AKR_VARIANT void SceneNode<C>::commit() {
        for (auto &shape : shapes) {
            AKR_ASSERT_THROW(shape);
            shape->commit();
        }
        AKR_ASSERT_THROW(camera);
        camera->commit();
    }
    AKR_VARIANT Scene<C> SceneNode<C>::compile(MemoryArena *arena) {
        Scene<C> scene;
        for (auto &shape : shapes) {
            meshviews.emplace_back(shape->compile(arena));
        }
        scene.meshes = meshviews;
        scene.camera = camera->compile(arena);
        return scene;
    }
    AKR_VARIANT void SceneNode<C>::render() {
        commit();
        MemoryArena arena;
        auto scene = compile(&arena);
        auto res = scene.camera.resolution();
        auto film = Film<C>(res);
        scene.sampler = Sampler<C>(arena.alloc<RandomSampler<C>>());
        auto embree_scene = EmbreeAccelerator<C>();
        scene.embree_scene = &embree_scene;
        scene.commit();
        auto integrator = cpu::Integrator<C>(arena.alloc<cpu::AmbientOcclusion<C>>());
        integrator.render(scene, &film);
        film.write_image(fs::path(output));
    }
    AKR_VARIANT void RegisterSceneNode<C>::register_nodes(py::module &m) {
        AKR_IMPORT_TYPES()
        py::class_<SceneNode<C>, SceneGraphNode<C>, std::shared_ptr<SceneNode<C>>>(m, "Scene")
            .def(py::init<>())
            .def_readwrite("variant", &SceneNode<C>::variant)
            .def_readwrite("camera", &SceneNode<C>::camera)
            .def_readwrite("output", &SceneNode<C>::output)
            .def("render", &SceneNode<C>::render)
            .def("add_mesh", &SceneNode<C>::add_mesh);
    }
    AKR_RENDER_CLASS(SceneNode)
    AKR_RENDER_STRUCT(RegisterSceneNode)
} // namespace akari