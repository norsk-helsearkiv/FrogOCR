#pragma once

#include "String.hpp"

#include <vector>

namespace frog::alto {

struct TextLine {

    int hpos{};
    int vpos{};
    int width{};
    int height{};
    std::optional<int> styleRefs;
    std::vector<String> strings;

	TextLine(String string);
	TextLine(std::vector<String> strings, std::optional<int> styleRefs = std::nullopt);
	TextLine() = default;

	int getMinimumVerticalPosition() const;
	int getMaxHeight() const;

	void fit();
	void setBoundingBox(int x, int y, int width, int height);

};

}
