#include "String.hpp"

namespace frog::alto {

String::String(std::string_view content, float confidence, int hpos, int vpos, int width, int height, std::vector<Glyph> glyphs_, std::optional<int> styleRefs_)
    : content{ content }, confidence{ confidence }, hpos{ hpos }, vpos{ vpos }, width{ width }, height{ height }, glyphs{ std::move(glyphs_) }, styleRefs{ styleRefs_ } {

}

bool String::isInsideRectangle(int left, int top, int right, int bottom) const {
    return hpos >= left && vpos >= top && hpos + width <= right && vpos + height <= bottom;
}

bool String::hasRectangleInside(int left, int top, int right, int bottom) const {
    return left >= hpos && top >= vpos && right <= hpos + width && bottom <= vpos + height;
}

}
