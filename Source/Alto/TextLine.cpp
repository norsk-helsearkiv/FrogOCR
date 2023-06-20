#include "TextLine.hpp"

namespace frog::alto {

TextLine::TextLine(String string) {
    strings.emplace_back(std::move(string));
}

TextLine::TextLine(std::vector<String> strings_, std::optional<int> styleRefs) : styleRefs{ styleRefs }, strings{ std::move(strings_) } {

}

int TextLine::getMinimumVerticalPosition() const {
    if (strings.empty()) {
        return 0;
    } else {
        auto minVerticalPosition = std::numeric_limits<int>::max();
        for (const auto& string: strings) {
            minVerticalPosition = std::min(minVerticalPosition, string.vpos);
        }
        return minVerticalPosition;
    }
}

int TextLine::getMaxHeight() const {
    int maxHeight{ 0 };
    for (const auto& string: strings) {
        maxHeight = std::max(maxHeight, string.height);
    }
    return maxHeight;
}

void TextLine::fit() {
    if (strings.empty()) {
        hpos = 0;
        vpos = 0;
        width = 0;
        height = 0;
    } else {
        hpos = std::numeric_limits<int>::max();
        vpos = std::numeric_limits<int>::max();
        int right{ 0 };
        int bottom{ 0 };
        for (const auto& string: strings) {
            hpos = std::min(hpos, string.hpos);
            vpos = std::min(vpos, string.vpos);
            right = std::max(right, string.hpos + string.width);
            bottom = std::max(bottom, string.vpos + string.height);
        }
        width = right - hpos;
        height = bottom - vpos;
    }
}

void TextLine::setBoundingBox(int x, int y, int width_, int height_) {
    hpos = x;
    vpos = y;
    width = width_;
    height = height_;
}

}
