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
#include <akari/core/nodes/mesh.h>
#include <akari/core/nodes/material.h>
#include <akari/core/resource.h>
#include <akari/core/mesh.h>
#include <akari/core/misc.h>
#include <akari/core/logger.h>

namespace akari {
    AKR_VARIANT class AkariMesh : public MeshNode<C> {
      public:
        std::string path;
        AkariMesh() = default;
        AkariMesh(std::string path) : path(path) {}
        std::shared_ptr<Mesh> mesh;
        std::vector<std::shared_ptr<MaterialNode<C>>> materials;
        void commit() override {
            auto exp = resource_manager()->load_path<BinaryGeometry>(path);
            if (exp) {
                mesh = exp.extract_value()->mesh();
            } else {
                auto err = exp.extract_error();
                error("error loading {}: {}", path, err.what());
                throw std::runtime_error("Error loading mesh");
            }
        }
        MeshInstance<C> compile(MemoryArena<> *arena) override {
            commit();
            MeshInstance<C> instance;
            instance.indices = mesh->indices.view();
            instance.material_indices = mesh->material_indices.view();
            instance.normals = mesh->normals.view();
            instance.texcoords = mesh->texcoords.view();
            instance.vertices = mesh->vertices.view();
            // AKR_ASSERT_THROW(mesh->material_indices.size() == materials.size());
            instance.materials = {arena->allocN<const Material<C> *>(materials.size()), materials.size()};
            for (size_t i = 0; i < materials.size(); i++) {
                instance.materials[i] = (materials[i]->compile(arena));
            }
            return instance;
        }
        void set_material(uint32_t index, const std::shared_ptr<MaterialNode<C>> &mat) {
            AKR_ASSERT(mat);
            if (index >= materials.size()) {
                materials.resize(index + 1);
            }
            materials[index] = mat;
        }
        void object_field(sdl::Parser &parser, sdl::ParserContext &ctx, const std::string &field,
                          const sdl::Value &value) override {
            if (field == "path") {
                path = value.get<std::string>().value();
            } else if (field == "materials") {
                AKR_ASSERT_THROW(value.is_array());
                for (auto mat : value) {
                    // debug("{}", typeid(*value.object()).name());
                    auto m = dyn_cast<MaterialNode<C>>(mat.object());
                    AKR_ASSERT(m);
                    materials.emplace_back(m);
                }
            }
        }
    };
#if 0
    AKR_VARIANT class OBJMesh : public MeshNode<C> {
        std::string loaded;

      public:
        AKR_IMPORT_TYPES()
        OBJMesh() = default;
        OBJMesh(const std::string &path) : path(path) {}
        using MaterialNode = akari::MaterialNode<C>;
        std::string path;
        Mesh mesh;
        std::vector<std::shared_ptr<MaterialNode>> materials;
        void commit() override {
            if (loaded == path)
                return;
            (void)load_wavefront_obj(path);
        }
        void object_field(sdl::Parser &parser, sdl::ParserContext &ctx, const std::string &field,
                          const sdl::Value &value) override {
            AKR_ASSERT_THROW(false && "not implemented");
        }
        MeshInstance<C> compile(MemoryArena<> *) override {
            commit();
            MeshInstance<C> view;
            view.indices = mesh.indices;
            view.material_indices = mesh.material_indices;
            view.normals = mesh.normals;
            view.texcoords = mesh.texcoords;
            view.vertices = mesh.vertices;
            return view;
        }

      private:
        bool load_wavefront_obj(const fs::path &obj) {
            info("loading {}", fs::absolute(obj).string());
            loaded = obj.string();
            fs::path parent_path = fs::absolute(obj).parent_path();
            fs::path file = obj.filename();
            CurrentPathGuard _;
            if (!parent_path.empty())
                fs::current_path(parent_path);

            tinyobj::attrib_t attrib;
            std::vector<tinyobj::shape_t> shapes;
            std::vector<tinyobj::material_t> obj_materials;

            std::string err;

            std::string _file = file.string();

            bool ret = misc::LoadObj(&attrib, &shapes, &obj_materials, &err, _file.c_str());
            (void)ret;
            mesh.vertices.resize(attrib.vertices.size());
            std::memcpy(&mesh.vertices[0], &attrib.vertices[0], sizeof(float) * mesh.vertices.size());
            for (size_t s = 0; s < shapes.size(); s++) {
                // Loop over faces(polygon)
                size_t index_offset = 0;
                for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {

                    // auto mat = materials[shapes[s].mesh.material_ids[f]].name;
                    int fv = shapes[s].mesh.num_face_vertices[f];
                    Float3 triangle[3];
                    for (int v = 0; v < fv; v++) {
                        tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                        mesh.indices.push_back(idx.vertex_index);
                        triangle[v] = Float3(akari::load<PackedArray<Float, 3>>(&mesh.vertices[3 * idx.vertex_index]));
                    }

                    Float3 ng =
                        normalize(cross(Float3(triangle[1] - triangle[0]), Float3(triangle[2] - triangle[0])));
                    for (int v = 0; v < fv; v++) {
                        tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                        if (idx.normal_index < 0) {
                            mesh.normals.emplace_back(ng[0]);
                            mesh.normals.emplace_back(ng[1]);
                            mesh.normals.emplace_back(ng[2]);
                        } else {
                            mesh.normals.emplace_back(attrib.normals[3 * idx.normal_index + 0]);
                            mesh.normals.emplace_back(attrib.normals[3 * idx.normal_index + 1]);
                            mesh.normals.emplace_back(attrib.normals[3 * idx.normal_index + 2]);
                        }
                        if (idx.texcoord_index < 0) {
                            mesh.texcoords.emplace_back(v > 0);
                            mesh.texcoords.emplace_back(v % 2 == 0);
                        } else {
                            mesh.texcoords.emplace_back(attrib.texcoords[2 * idx.texcoord_index + 0]);
                            mesh.texcoords.emplace_back(attrib.texcoords[2 * idx.texcoord_index + 1]);
                        }
                    }

                    index_offset += fv;
                }
            }
            info("loaded {} triangles, {} vertices", mesh.indices.size() / 3, mesh.vertices.size() / 3);
            return true;
        }
    };
#endif
    AKR_VARIANT void RegisterMeshNode<C>::register_nodes() {
        AKR_IMPORT_TYPES();
        register_node<C, AkariMesh<C>>("AkariMesh");
    }

    AKR_VARIANT void RegisterMeshNode<C>::register_python_nodes(py::module &m) {
#ifdef AKR_ENABLE_PYTHON
        py::class_<MeshNode<C>, SceneGraphNode<C>, std::shared_ptr<MeshNode<C>>>(m, "Mesh").def("commit",
                                                                                                &MeshNode<C>::commit);
        py::class_<OBJMesh<C>, MeshNode<C>, std::shared_ptr<OBJMesh<C>>>(m, "OBJMesh")
            .def(py::init<>())
            .def(py::init<const std::string &>())
            .def("commit", &OBJMesh<C>::commit)
            .def_readwrite("path", &OBJMesh<C>::path);
        py::class_<AkariMesh<C>, MeshNode<C>, std::shared_ptr<AkariMesh<C>>>(m, "AkariMesh")
            .def(py::init<>())
            .def(py::init<const std::string &>())
            .def("commit", &AkariMesh<C>::commit)
            .def("set_material", &AkariMesh<C>::set_material)
            .def_readwrite("path", &AkariMesh<C>::path);
        m.def("load_mesh", [](const std::string &path) { return std::make_shared<AkariMesh<C>>(path); });
#endif
    }

    AKR_RENDER_STRUCT(RegisterMeshNode)
} // namespace akari