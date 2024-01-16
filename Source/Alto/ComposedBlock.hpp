#pragma once

#include "TextBlock.hpp"

namespace frog::alto {

struct ComposedBlock {

    std::string id;
    int hpos{};
    int vpos{};
    int width{};
    int height{};
    std::vector<TextBlock> textBlocks;
    std::vector<std::string> processingRefs;

	ComposedBlock(std::vector<TextBlock> textBlocks);
	ComposedBlock(TextBlock textBlock);
	ComposedBlock() = default;

	void fit();
	void setBoundingBox(int x, int y, int width, int height);

};

}
