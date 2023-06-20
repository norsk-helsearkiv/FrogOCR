#include "TextBlock.hpp"

namespace frog::alto {

TextBlock::TextBlock(TextLine textLine) {
    textLines.emplace_back(std::move(textLine));
}

TextBlock::TextBlock(std::vector<TextLine> textLines_) : textLines{ std::move(textLines_) } {

}

void TextBlock::fit() {
    if (textLines.empty()) {
        hpos = 0;
        vpos = 0;
        width = 0;
        height = 0;
    } else {
        hpos = std::numeric_limits<int>::max();
        vpos = std::numeric_limits<int>::max();
        int right{ 0 };
        int bottom{ 0 };
        for (auto& textLine: textLines) {
            hpos = std::min(hpos, textLine.hpos);
            vpos = std::min(vpos, textLine.getMinimumVerticalPosition());
            right = std::max(right, textLine.hpos + textLine.width);
            bottom = std::max(bottom, textLine.getMinimumVerticalPosition() + textLine.getMaxHeight());
        }
        width = right - hpos;
        height = bottom - vpos;
    }
}

void TextBlock::setBoundingBox(int x, int y, int width_, int height_) {
    hpos = x;
    vpos = y;
    width = width_;
    height = height_;
}

}
