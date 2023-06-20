#include "ComposedBlock.hpp"
#include "TextBlock.hpp"

namespace frog::alto {

ComposedBlock::ComposedBlock(TextBlock textBlock) {
    textBlocks.emplace_back(std::move(textBlock));
}

ComposedBlock::ComposedBlock(std::vector<TextBlock> textBlocks_) : textBlocks{std::move(textBlocks_) } {

}

void ComposedBlock::fit() {
	if (textBlocks.empty()) {
		hpos = 0;
		vpos = 0;
		width = 0;
		height = 0;
	} else {
		hpos = std::numeric_limits<int>::max();
		vpos = std::numeric_limits<int>::max();
		int right{ 0 };
		int bottom{ 0 };
		for (const auto& textBlock : textBlocks) {
			hpos = std::min(hpos, textBlock.hpos);
			vpos = std::min(vpos, textBlock.vpos);
			right = std::max(right, textBlock.hpos + textBlock.width);
			bottom = std::max(bottom, textBlock.vpos + textBlock.height);
		}
		width = right - hpos;
		height = bottom - vpos;
	}
}

void ComposedBlock::setBoundingBox(int x, int y, int width_, int height_) {
	hpos = x;
	vpos = y;
	width = width_;
	height = height_;
}

}
