#pragma once

#include "Confidence.hpp"

#include <string>
#include <vector>

namespace frog::ocr {

struct Variant {
	std::string text;
	Confidence confidence;
};

struct Symbol {
	int x{};
	int y{};
	int width{};
	int height{};
	std::string text;
	Confidence confidence;
	std::vector<Variant> variants;
};

}
