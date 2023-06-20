#pragma once

#include <string>

namespace frog::ansi {

constexpr char escape = '\x1B';
constexpr char control_sequence_introducer = '[';

// todo: this can probably be constexpr in c++20
inline std::string call(char function, std::initializer_list<int> arguments) {
	std::string result{ escape, control_sequence_introducer };
	if (arguments.size() > 0) {
		for (const int argument : arguments) {
			result += std::to_string(argument) + ';';
		}
		result.back() = function;
	} else {
		result += function;
	}
	return result;
}

inline std::string call(char function, int argument) {
	return std::string{ escape, control_sequence_introducer } + std::to_string(argument) + function;
}

namespace sgr { // select graphics rendition

constexpr char name = 'm';

inline std::string call(int argument) {
	return ansi::call(name, argument);
}

constexpr int reset{ 0 };
constexpr int bold{ 1 };
constexpr int italic{ 3 };
constexpr int underline{ 4 };

constexpr int black_text{ 30 };
constexpr int red_text{ 31 };
constexpr int green_text{ 32 };
constexpr int yellow_text{ 33 };
constexpr int blue_text{ 34 };
constexpr int magenta_text{ 35 };
constexpr int cyan_text{ 36 };
constexpr int white_text{ 37 };

constexpr int bright_black_text{ 90 };
constexpr int bright_red_text{ 91 };
constexpr int bright_green_text{ 92 };
constexpr int bright_yellow_text{ 93 };
constexpr int bright_blue_text{ 94 };
constexpr int bright_magenta_text{ 95 };
constexpr int bright_cyan_text{ 96 };
constexpr int bright_white_text{ 97 };

constexpr int black_background{ 40 };
constexpr int red_background{ 41 };
constexpr int green_background{ 42 };
constexpr int yellow_background{ 43 };
constexpr int blue_background{ 44 };
constexpr int magenta_background{ 45 };
constexpr int cyan_background{ 46 };
constexpr int white_background{ 47 };

constexpr int bright_black_background{ 100 };
constexpr int bright_red_background{ 101 };
constexpr int bright_green_background{ 102 };
constexpr int bright_yellow_background{ 103 };
constexpr int bright_blue_background{ 104 };
constexpr int bright_magenta_background{ 105 };
constexpr int bright_cyan_background{ 106 };
constexpr int bright_white_background{ 107 };

}

}
