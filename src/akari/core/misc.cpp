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
#define TINYOBJLOADER_IMPLEMENTATION
#include <akari/common/platform.h>
#include <akari/core/misc.h>
namespace akari::misc {
    AKR_EXPORT bool LoadObj(tinyobj::attrib_t *attrib, std::vector<tinyobj::shape_t> *shapes,
                 std::vector<tinyobj::material_t> *materials, std::string *err, const char *filename,
                 const char *mtl_basedir, bool trianglulate) {
        return tinyobj::LoadObj(attrib, shapes, materials, err, filename, mtl_basedir, trianglulate);
    }
} // namespace akari::misc