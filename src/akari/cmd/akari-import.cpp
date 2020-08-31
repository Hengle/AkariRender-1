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
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <fstream>
#include <cxxopts.hpp>
#include <akari/core/logger.h>
#include <akari/core/mesh.h>
#include <tiny_obj_loader.h>
using namespace akari;
std::shared_ptr<Mesh> load_wavefront_obj(const fs::path &path) {
    AKR_IMPORT_CORE_TYPES_WITH(float)
    info("loading {}\n", fs::absolute(path).string());
    std::shared_ptr<Mesh> mesh;
    fs::path parent_path = fs::absolute(path).parent_path();
    fs::path file = path.filename();
    CurrentPathGuard _;
    if (!parent_path.empty())
        fs::current_path(parent_path);

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> obj_materials;

    std::string err;

    std::string _file = file.string();

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &obj_materials, &err, _file.c_str());
    if (!ret) {
        error("error: {}\n", err);
        return nullptr;
    }
    mesh = std::make_shared<Mesh>();
    mesh->vertices.resize(attrib.vertices.size());
    std::memcpy(&mesh->vertices[0], &attrib.vertices[0], sizeof(float) * mesh->vertices.size());
    for (size_t s = 0; s < shapes.size(); s++) {
        // Loop over faces(polygon)
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {

            // auto mat = materials[shapes[s].mesh.material_ids[f]].name;
            int fv = shapes[s].mesh.num_face_vertices[f];
            Point3f triangle[3];
            for (int v = 0; v < fv; v++) {
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                mesh->indices.push_back(idx.vertex_index);
                triangle[v] = Point3f(load<PackedArray<Float, 3>>(&mesh->vertices[3 * idx.vertex_index]));
            }
            mesh->material_indices.emplace_back(-1);
            Normal3f ng = normalize(cross(Vector3f(triangle[1] - triangle[0]), Vector3f(triangle[2] - triangle[0])));
            for (int v = 0; v < fv; v++) {
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                if (idx.normal_index < 0) {
                    mesh->normals.emplace_back(ng[0]);
                    mesh->normals.emplace_back(ng[1]);
                    mesh->normals.emplace_back(ng[2]);
                } else {
                    mesh->normals.emplace_back(attrib.normals[3 * idx.normal_index + 0]);
                    mesh->normals.emplace_back(attrib.normals[3 * idx.normal_index + 1]);
                    mesh->normals.emplace_back(attrib.normals[3 * idx.normal_index + 2]);
                }
                if (idx.texcoord_index < 0) {
                    mesh->texcoords.emplace_back(v > 0);
                    mesh->texcoords.emplace_back(v % 2 == 0);
                } else {
                    mesh->texcoords.emplace_back(attrib.texcoords[2 * idx.texcoord_index + 0]);
                    mesh->texcoords.emplace_back(attrib.texcoords[2 * idx.texcoord_index + 1]);
                }
            }

            index_offset += fv;
        }
        
    }
    info("loaded {} triangles, {} vertices\n", mesh->indices.size() / 3, mesh->vertices.size() / 3);
    return mesh;
}

int main(int argc, const char **argv) {
    try {
        cxxopts::Options options("akari-import", " - Import OBJ meshes to akari scene description file");
        options.positional_help("input output").show_positional_help();
        {
            auto opt = options.allow_unrecognised_options().add_options();
            opt("i,input", "Input Scene Description File", cxxopts::value<std::string>());
            opt("o,output", "Input Scene Description File", cxxopts::value<std::string>());
            opt("help", "Show this help");
        }
        options.parse_positional({"input", "output"});
        auto result = options.parse(argc, argv);
        if (!result.count("input")) {
            fatal("Input file must be provided\n");
            std::cout << options.help() << std::endl;
            exit(0);
        }
        if (!result.count("output")) {
            fatal("Output file must be provided\n");
            std::cout << options.help() << std::endl;
            exit(0);
        }
        if (result.arguments().empty() || result.count("help")) {
            std::cout << options.help() << std::endl;
            exit(0);
        }
        auto inputFilename = result["input"].as<std::string>();
        auto outputFilename = result["output"].as<std::string>();
        auto mesh = load_wavefront_obj(fs::path(inputFilename));
        auto res = std::make_shared<BinaryGeometry>(mesh);
        auto mesh_file = fs::path(inputFilename).filename().concat(".mesh");
        auto out_dir = fs::absolute(outputFilename).parent_path();
        res->save(fs::path(out_dir / mesh_file));
        std::ofstream os(outputFilename);
        os << "mesh = load_mesh(" << mesh_file << ")\n";
        os << "export(mesh)";
    } catch (std::exception &e) {
        fatal("Exception: {}\n", e.what());
        exit(1);
    }
}