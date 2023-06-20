#pragma once

#include "Line.hpp"

namespace frog::ocr {

struct Paragraph {
	int x{};
	int y{};
	int width{};
	int height{};
	std::vector<Line> lines;
	Confidence confidence;
};

}
