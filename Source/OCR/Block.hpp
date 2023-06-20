#pragma once

#include "Paragraph.hpp"

namespace frog::ocr {

struct Block {
	int x{};
	int y{};
	int width{};
	int height{};
	std::vector<Paragraph> paragraphs;
	Confidence confidence;
};

}
