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
#include <string>
#include <vector>
#include <memory>
#include <akari/core/platform.h>
#include <akari/core/error.hpp>
namespace akari::asl {
    struct CompileOptions {
        enum OptLevel {
            OFF,
            O0,
            O1,
            O2,
            O3,
        };
        OptLevel opt_level = O2;
        const char *backend = "llvm";
    };
    class Program {
      public:
        template <typename ShadingResult, typename ShadingArg> std::function<ShadingResult(ShadingArg)> get() {
            auto fp = reinterpret_cast<void (*)(ShadingResult *, const ShadingArg *)>(get_entry());
            return [=](ShadingArg arg) -> ShadingResult {
                ShadingArg _ = arg;
                ShadingResult res;
                fp(&res, &_);
                return res;
            };
        }
        virtual void *get_entry() = 0;
    };

    struct TranslationUnit {
        std::string filename;
        std::string source;
    };
    AKR_EXPORT Expected<std::shared_ptr<Program>> compile(const std::vector<TranslationUnit> &,
                                                          CompileOptions opt = CompileOptions());

} // namespace akari::asl