#pragma once

#include "Filesystem.hpp"
#include "Formatting.hpp"
#include "AnsiEscape.hpp"

#include <mutex>
#include <filesystem>
#include <experimental/source_location>

namespace frog::log {

struct source_with_format {
	const std::string_view format;
	const std::experimental::source_location source;
	consteval source_with_format(auto format, const std::experimental::source_location& source = std::experimental::source_location::current()) : format{ format }, source{ source } {}
};

void print(const source_with_format& source, std::string_view type, std::string message);

template<typename... Args>
void info(source_with_format format, Args&&... args) {
    print(format, "info", fmt::vformat(format.format, fmt::make_format_args(args...)));
}

template<typename... Args>
void notice(source_with_format format, Args&&... args) {
    print(format, "notice", fmt::vformat(format.format, fmt::make_format_args(args...)));
}

template<typename... Args>
void warning(source_with_format format, Args&&... args) {
    print(format, "warning", fmt::vformat(format.format, fmt::make_format_args(args...)));
}

template<typename... Args>
void error(source_with_format format, Args&&... args) {
    print(format, "error", fmt::vformat(format.format, fmt::make_format_args(args...)));
}

}
