#pragma once

#include "TextLine.hpp"

namespace frog::alto {

struct TextBlock {

    int hpos{};
    int vpos{};
    int width{};
    int height{};
    std::vector<TextLine> textLines;

	TextBlock(TextLine textLine);
	TextBlock(std::vector<TextLine> textLines);
	TextBlock() = default;

	void fit();
	void setBoundingBox(int x, int y, int width, int height);

};

}
