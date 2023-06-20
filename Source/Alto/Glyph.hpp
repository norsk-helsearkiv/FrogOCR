#pragma once

#include "Variant.hpp"

namespace frog::alto {

struct Glyph {

    std::string content;
    int hpos{};
    int vpos{};
    int width{};
    int height{};
    float confidence{};
    std::vector<Variant> variants;

    Glyph() = default;

    Glyph(std::string_view content, float confidence, int hpos, int vpos, int width, int height, std::vector<Variant> variants_)
        : content{ content }, confidence{ confidence }, hpos{ hpos }, vpos{ vpos }, width{ width }, height{ height }, variants{ std::move(variants_) } {

    }

};

}
