#include "PrintSpace.hpp"
#include "TextLine.hpp"
#include "String.hpp"

namespace frog::alto {

std::vector<const String*> PrintSpace::getStringsInArea(int x, int y, int width, int height) const {
	std::vector<const String*> strings;
	for (auto& composedBlock : composedBlocks) {
		for (auto& textBlock : composedBlock.textBlocks) {
			for (auto& textLine : textBlock.textLines) {
				for (auto& string : textLine.strings) {
					if (string.isInsideRectangle(x, y, x + width, y + height) || string.hasRectangleInside(x, y, x + width, y + height)) {
						strings.push_back(&string);
					}
				}
			}
		}
	}
	return strings;
}

TextLine* PrintSpace::findClosestTextLine(int x, int y, int width, int height, int required_min_vertical_distance) {
	TextLine* closestTextLine{ nullptr };
	int best_x_difference{};
	int best_y_difference{};
	int best_width_difference{};
	int best_height_difference{};
	for (auto& composedBlock : composedBlocks) {
		for (auto& textBlock : composedBlock.textBlocks) {
			for (auto& textLine : textBlock.textLines) {
				int x_difference = std::abs(textLine.hpos - x);
				int y_difference = std::abs(textLine.getMinimumVerticalPosition() - y);
				int width_difference = std::abs(textLine.width - width);
				int height_difference = std::abs(textLine.getMaxHeight() - height);
				if (!closestTextLine) {
					closestTextLine = &textLine;
					best_x_difference = x_difference;
					best_y_difference = y_difference;
					best_width_difference = width_difference;
					best_height_difference = height_difference;
					continue;
				}
				if (best_y_difference > y_difference) {
					closestTextLine = &textLine;
					best_x_difference = x_difference;
					best_y_difference = y_difference;
					best_width_difference = width_difference;
					best_height_difference = height_difference;
				}
			}
		}
	}
	if (best_y_difference > required_min_vertical_distance) {
		return nullptr;
	}
	return closestTextLine;
}

}
