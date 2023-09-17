#pragma once

#include "Glyph.hpp"

namespace frog::alto {

struct String {

    std::string content;
    int hpos{};
    int vpos{};
    int width{};
    int height{};
    float rotation{};
    float confidence{};
    std::vector<Glyph> glyphs;
    std::optional<int> styleRefs;

    String(std::string_view content, float confidence, int hpos, int vpos, int width, int height, float rotation, std::vector<Glyph> glyphs, std::optional<int> styleRefs);
    String() = default;

    // Does the string bounding box fit completely inside the rectangle?
    bool isInsideRectangle(int left, int top, int right, int bottom) const;

    // Does the rectangle fit completely inside the string bounding box?
    bool hasRectangleInside(int left, int top, int right, int bottom) const;

};

}
