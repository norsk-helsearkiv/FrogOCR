#pragma once

#include "Word.hpp"

namespace frog::ocr {

struct Line {
	int x{};
	int y{};
	int width{};
	int height{};
	std::vector<Word> words;
	Confidence confidence;
    std::optional<int> styleRefs;
};

}
