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
#ifdef AKR_ENABLE_PYTHON
#    include <akari/common/math.h>
#    include <akari/core/logger.h>
#    ifdef AKR_ENABLE_PYTHON
#        include <pybind11/pybind11.h>
#        include <pybind11/pybind11.h>
#        include <pybind11/embed.h>
#    endif
namespace akari {
    namespace py = pybind11;

    AKR_VARIANT
    struct RegisterMathFunction {
        static void register_math_functions(py::module &m);
    };
    void register_utility(py::module &m);
    AKR_EXPORT void register_module_akari(py::module &m);
} // namespace akari

#endif