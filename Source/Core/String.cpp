#include "String.hpp"

#include <algorithm>
#include <locale>

namespace frog {

std::vector<std::string_view> split_string_view(std::string_view string, std::string_view symbols) {
    if (string.empty()) {
        return {};
    }
    std::vector<std::string_view> result;
    std::size_t start{ 0 };
    std::size_t next{ string.find_first_of(symbols) };
    while (next != std::string_view::npos) {
        result.push_back(string.substr(start, next - start));
        start = next + 1;
        next = string.find_first_of(symbols, start);
    }
    result.push_back(string.substr(start, next - start));
    return result;
}

std::vector<std::string> split_string(std::string_view string, std::string_view symbols) {
    if (string.empty()) {
        return {};
    }
    std::vector<std::string> result;
    std::size_t start{ 0 };
    std::size_t next{ string.find_first_of(symbols) };
    while (next != std::string_view::npos) {
        result.emplace_back(string.substr(start, next - start));
        start = next + 1;
        next = string.find_first_of(symbols, start);
    }
    result.emplace_back(string.substr(start, next - start));
    return result;
}

std::string_view trim_string_view_left(std::string_view string, std::string_view characters) {
    if (const auto offset = string.find_first_not_of(characters.data()); offset != std::string_view::npos) {
        return { string.data() + offset, string.size() - offset };
    } else {
        return {};
    }
}

std::string_view trim_string_view_right(std::string_view string, std::string_view characters) {
    if (const auto offset = string.find_last_not_of(characters.data()); offset != std::string_view::npos) {
        return { string.data(), offset + 1 };
    } else {
        return {};
    }
}

std::string_view trim_string_view(std::string_view string, std::string_view characters) {
    return trim_string_view_left(trim_string_view_right(string, characters), characters);
}

std::string string_to_lowercase(std::string string) {
    std::ranges::transform(string, string.begin(), [](const auto character) {
        return std::tolower(character);
    });
    return string;
}

std::string string_to_uppercase(std::string string) {
    std::ranges::transform(string, string.begin(), [](const auto character) {
        return std::toupper(character);
    });
    return string;
}

void erase_substring(std::string& string, std::string_view substring, string_comparison comparison) {
    if (substring.empty()) {
        return;
    }
    switch (comparison) {
        case string_comparison::case_sensitive:
            if (const auto index = string.find(substring); index != std::string::npos) {
                string.erase(string.find(substring), substring.size());
                erase_substring(string, substring, comparison);
            }
            break;

        case string_comparison::case_insensitive:
            if (const auto index = string_to_lowercase(string).find(substring); index != std::string::npos) {
                string.erase(index, substring.size());
                erase_substring(string, substring, comparison);
            }
            break;
    }
}

std::size_t replace_substring(std::string& string, std::string_view substring, std::string_view replace_with, std::size_t offset, std::size_t max_replacements) {
    if (substring.empty()) {
        return offset;
    }
    auto new_offset = offset;
    auto index = string.find(substring, offset);
    while (index != std::string::npos && max_replacements != 0) {
        new_offset = index;
        string.replace(index, substring.size(), replace_with);
        index = string.find(substring, index + replace_with.size());
        max_replacements--;
    }
    return new_offset;
}

std::string merge_strings(const std::vector<std::string>& strings, std::string_view glue) {
    std::string result;
    result.reserve(strings.size() * 20);
    for (std::size_t i{ 0 }; i < strings.size(); i++) {
        result += strings[i];
        if (i + 1 < strings.size()) {
            result += glue;
        }
    }
    return result;
}

std::string merge_strings(const std::vector<std::string_view>& strings, std::string_view glue) {
    std::string result;
    result.reserve(strings.size() * 20);
    for (std::size_t i{ 0 }; i < strings.size(); i++) {
        result += strings[i];
        if (i + 1 < strings.size()) {
            result += glue;
        }
    }
    return result;
}

int levenshtein(std::string_view word1, std::string_view word2) {
    const auto size1 = static_cast<int>(word1.size());
    const auto size2 = static_cast<int>(word2.size());
    std::vector<std::vector<int>> verificationMatrix; // 2D array which will store the calculated distance.
    for (int i{ 0 }; i <= size1; i++) {
        auto& v = verificationMatrix.emplace_back();
        for (int j{ 0 }; j <= size2; j++) {
            v.emplace_back(0);
        }
    }

    // If one of the words has zero length, the distance is equal to the size of the other word.
    if (size1 == 0) {
        return size2;
    }
    if (size2 == 0) {
        return size1;
    }

    // Sets the first row and the first column of the verification matrix with the numerical order from 0 to the length of each word.
    for (int i{ 0 }; i <= size1; i++) {
        verificationMatrix[i][0] = i;
    }
    for (int j{ 0 }; j <= size2; j++) {
        verificationMatrix[0][j] = j;
    }

    // Verification step / matrix filling.
    for (int i{ 1 }; i <= size1; i++) {
        for (int j{ 1 }; j <= size2; j++) {
            // Sets the modification cost.
            // 0 means no modification (i.e. equal letters)
            // 1 means that a modification is needed (i.e. unequal letters).
            const int cost = (word2[j - 1] == word1[i - 1]) ? 0 : 1;

            // Sets the current position of the matrix as the minimum value between a (deletion), b (insertion) and c (substitution).
            // a = the upper adjacent value plus 1: verificationMatrix[i - 1][j] + 1
            // b = the left adjacent value plus 1: verificationMatrix[i][j - 1] + 1
            // c = the upper left adjacent value plus the modification cost: verificationMatrix[i - 1][j - 1] + cost
            verificationMatrix[i][j] = std::min(std::min(verificationMatrix[i - 1][j] + 1, verificationMatrix[i][j - 1] + 1), verificationMatrix[i - 1][j - 1] + cost);
        }
    }

    // The last position of the matrix will contain the Levenshtein distance.
    return verificationMatrix[size1][size2];
}

std::string current_date_string() {
    tm localTime{};
    auto now = std::time(nullptr);
#ifdef WIN32
    localtime_s(&time_data, &time);
#else
    localtime_r(&now, &localTime);
#endif
    char buffer[64]{};
    std::strftime(buffer, 64, "%F", &localTime);
    return buffer;
}

std::string current_time_string() {
    tm localTime{};
    auto now = std::time(nullptr);
#ifdef WIN32
    localtime_s(&time_data, &time);
#else
    localtime_r(&now, &localTime);
#endif
    char buffer[64]{};
    std::strftime(buffer, 64, "%X", &localTime);
    return buffer;
}

std::string current_timestamp_string() {
    tm localTime{};
    auto now = std::time(nullptr);
#ifdef WIN32
    localtime_s(&time_data, &time);
#else
    localtime_r(&now, &localTime);
#endif
    char buffer[64]{};
    std::strftime(buffer, 64, "%F %X", &localTime);
    return buffer;
}

}
