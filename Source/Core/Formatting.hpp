#pragma once

#include "String.hpp"

#include <fmt/format.h>
#include <fmt/ostream.h>

namespace fmt {

template<>
struct formatter<std::filesystem::path>: formatter<std::string_view> {
    auto format(const std::filesystem::path& path, auto& context) {
        return formatter<std::string_view>::format(frog::path_to_string(path), context);
    }
};

template<>
struct formatter<std::u8string>: formatter<std::string_view> {
    auto format(const std::u8string& value, auto& context) {
        return formatter<std::string_view>::format(frog::u8string_to_string(value), context);
    }
};

template<>
struct formatter<std::error_code>: formatter<std::string_view> {
    auto format(std::error_code error, auto& context) {
        return formatter<std::string_view>::format(fmt::format("{} ({})", error.message(), error.value()), context);
    }
};

}
