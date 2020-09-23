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

#include <fstream>
#include "string-utils.h"
#include <fmt/format.h>
int main(int argc, char **argv) {
    if (argc != 3) {
        fmt::print(stderr, "Usage: akari-configure conf out\n");
        std::exit(1);
    }
    std::ifstream in(argv[1]);
    std::string str((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    str = akari::tool::jsonc_to_json(str);
    std::ofstream out(argv[2]);
    using namespace nlohmann;
    try {
        json config = json::parse(str);
        json variants = config["variants"];
        for (auto &variant : variants.items()) {
            auto &settings = variant.value();
            settings["Spectrum"] = std::regex_replace(settings["Spectrum"].get<std::string>(), std::regex("Float"),
                                                      settings["Float"].get<std::string>());
        }
        out << "/******  AUTO GENERATED BY akari-configure ******/\n";
        out << R"(#pragma once
#include <type_traits>
namespace akari {
    template<typename Float, int N> struct Color;
    template <typename Float_, typename Spectrum_> struct Config;
)";
        out << "#define AKR_CORE_STRUCT(Name) ";
        for (auto enabled : config["enabled"]) {
            out << fmt::format("template struct Name<{}>;",
                               variants[enabled.get<std::string>()]["Float"].get<std::string>());
        }
        out << "\n";
        out << "#define AKR_CORE_CLASS(Name) ";
        for (auto enabled : config["enabled"]) {
            out << fmt::format("template class Name<{}>;",
                               variants[enabled.get<std::string>()]["Float"].get<std::string>());
        }
        out << "\n";
        out << "#define AKR_RENDER_STRUCT(Name) ";
        for (auto enabled : config["enabled"]) {
            out << fmt::format("template struct Name<Config<{}, {}>>;",
                               variants[enabled.get<std::string>()]["Float"].get<std::string>(),
                               variants[enabled.get<std::string>()]["Spectrum"].get<std::string>());
        }
        out << "\n";
        out << "#define AKR_RENDER_CLASS(Name) ";
        for (auto enabled : config["enabled"]) {
            out << fmt::format("template class Name<Config<{}, {}>>;",
                               variants[enabled.get<std::string>()]["Float"].get<std::string>(),
                               variants[enabled.get<std::string>()]["Spectrum"].get<std::string>());
        }
        out << "\n";
        out << "template<typename C>constexpr const char * get_variant_string(){\n";
        for (auto enabled : config["enabled"]) {
            out << fmt::format(R"(    if constexpr(std::is_same_v<Config<{},{}>, C>)return "{}";)",
                               variants[enabled.get<std::string>()]["Float"].get<std::string>(),
                               variants[enabled.get<std::string>()]["Spectrum"].get<std::string>(),
                               enabled.get<std::string>());
        }
        out << "\n    return \"unknown\";\n}\n";
        out << "#define AKR_INVOKE_VARIANT(variant, func, ...) ([&](){\\\n";
        for (auto enabled : config["enabled"]) {
            out << fmt::format("    if (variant == \"{}\")\\\n", enabled.get<std::string>());
            out << fmt::format("        return func<Config<{}, {}>>(__VA_ARGS__);\\\n",
                               variants[enabled.get<std::string>()]["Float"].get<std::string>(),
                               variants[enabled.get<std::string>()]["Spectrum"].get<std::string>());
        }
        out << "    throw std::runtime_error(\"unsupported variant\");\\\n";
        out << "})()\n";
        out << "constexpr const char * enabled_variants[] = {";
        {
            std::vector<std::string> v;
            for (auto enabled : config["enabled"]) {
                v.emplace_back(fmt::format("\"{}\"", enabled.get<std::string>()));
            }
            out << akari::tool::join(",", v.begin(), v.end());
        }
        out << "};\n}";
    } catch (std::exception &e) {
        fmt::print(stderr, "{}\n", e.what());
        std::exit(1);
    }
}