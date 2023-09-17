#pragma once

#include <vector>
#include <string_view>
#include <optional>
#include <charconv>
#include <filesystem>
#include <sstream>
#include <iostream>

namespace frog {

enum class string_comparison { case_sensitive, case_insensitive };

inline std::string u8string_to_string(std::u8string_view string) {
    return { string.begin(), string.end() };
}

inline std::string path_to_string(const std::filesystem::path& path) {
    return u8string_to_string(path.u8string());
}

inline std::filesystem::path path_with_extension(const std::filesystem::path& path, std::string_view extension) {
    auto result = path;
    result.replace_extension(extension);
    return result;
}

template<typename T>
[[nodiscard]] std::optional<T> from_string(std::string_view string) {
    T result{};
    const auto status = std::from_chars(string.data(), string.data() + string.size(), result);
    return status.ec == std::errc{} ? result : std::optional<T>{};
}

std::vector<std::string_view> split_string_view(std::string_view string, std::string_view symbols);
std::vector<std::string> split_string(std::string_view string, std::string_view symbols);

std::string_view trim_string_view_left(std::string_view string, std::string_view characters);
std::string_view trim_string_view_right(std::string_view string, std::string_view characters);
std::string_view trim_string_view(std::string_view string, std::string_view characters);

std::string string_to_lowercase(std::string string);
std::string string_to_uppercase(std::string string);

void erase_substring(std::string& string, std::string_view substring, string_comparison comparison = string_comparison::case_sensitive);
std::size_t replace_substring(std::string& string, std::string_view substring, std::string_view replace_with, std::size_t offset = 0, std::size_t max_replacements = std::string::npos);

std::string merge_strings(const std::vector<std::string>& strings, std::string_view glue);
std::string merge_strings(const std::vector<std::string_view>& strings, std::string_view glue);

template<typename T>
struct is_pointer {
    static constexpr bool value{ false };
};

template<typename T>
struct is_pointer<T*> {
    static constexpr bool value{ true };
};

template<typename T>
struct is_pointer<std::shared_ptr<T>> {
    static constexpr bool value{ true };
};

template<typename Element, typename String = std::string>
[[nodiscard]] std::vector<String> to_strings(const std::vector<Element>& elements) {
    std::vector<String> values;
    for (const auto& element : elements) {
        if constexpr (is_pointer<Element>::value) {
            values.push_back(element->to_string());
        } else if constexpr (std::is_arithmetic_v<Element>) {
            values.push_back(std::to_string(element));
        } else {
            values.push_back(element.to_string());
        }
    }
    return values;
}

inline std::string to_xml_attribute(std::string value) {
    replace_substring(value, "&", "&amp;");
    replace_substring(value, "\"", "&quot;");
    replace_substring(value, "<", "&lt;");
    replace_substring(value, ">", "&gt;");
    return value;
}

int levenshtein(std::string_view word1, std::string_view word2);

std::string current_date_string();
std::string current_time_string();
std::string current_timestamp_string();

}
