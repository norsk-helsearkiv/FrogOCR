#pragma once

#include "Symbol.hpp"

#include <optional>
#include <vector>

namespace frog::ocr {

struct Word {
	int x{};
	int y{};
	int width{};
	int height{};
	std::vector<Symbol> symbols;
	std::string text;
	Confidence confidence;
    std::optional<int> styleRefs;
};

}
