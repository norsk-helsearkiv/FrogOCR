#include "Log.hpp"
#include "AnsiEscape.hpp"

#include <chrono>
#include <typeinfo>
#include <array>
#include <vector>
#include <iostream>
#include <syncstream>

namespace frog::ansi {

// { output, replacement length }
std::pair<std::string, std::size_t> format_ansi_text_graphics(std::string_view string, std::size_t offset) {
    static const std::vector<std::pair<std::string_view, std::string>> mapping{
        { "reset",                    sgr::call(sgr::reset) },
        { "bold",                     sgr::call(sgr::bold) },
        { "italic",                   sgr::call(sgr::italic) },
        { "underline",                sgr::call(sgr::underline) },

        { "black",                    sgr::call(sgr::black_text) },
        { "red",                      sgr::call(sgr::red_text) },
        { "green",                    sgr::call(sgr::green_text) },
        { "yellow",                   sgr::call(sgr::yellow_text) },
        { "blue",                     sgr::call(sgr::blue_text) },
        { "magenta",                  sgr::call(sgr::magenta_text) },
        { "cyan",                     sgr::call(sgr::cyan_text) },
        { "white",                    sgr::call(sgr::white_text) },

        { "light-black",              sgr::call(sgr::bright_black_text) },
        { "light-red",                sgr::call(sgr::bright_red_text) },
        { "light-green",              sgr::call(sgr::bright_green_text) },
        { "light-yellow",             sgr::call(sgr::bright_yellow_text) },
        { "light-blue",               sgr::call(sgr::bright_blue_text) },
        { "light-magenta",            sgr::call(sgr::bright_magenta_text) },
        { "light-cyan",               sgr::call(sgr::bright_cyan_text) },
        { "light-white",              sgr::call(sgr::bright_white_text) },

        { "black-background",         sgr::call(sgr::black_background) },
        { "red-background",           sgr::call(sgr::red_background) },
        { "green-background",         sgr::call(sgr::green_background) },
        { "yellow-background",        sgr::call(sgr::yellow_background) },
        { "blue-background",          sgr::call(sgr::blue_background) },
        { "magenta-background",       sgr::call(sgr::magenta_background) },
        { "cyan-background",          sgr::call(sgr::cyan_background) },
        { "white-background",         sgr::call(sgr::white_background) },

        { "light-black-background",   sgr::call(sgr::bright_black_background) },
        { "light-red-background",     sgr::call(sgr::bright_red_background) },
        { "light-green-background",   sgr::call(sgr::bright_green_background) },
        { "light-yellow-background",  sgr::call(sgr::bright_yellow_background) },
        { "light-blue-background",    sgr::call(sgr::bright_blue_background) },
        { "light-magenta-background", sgr::call(sgr::bright_magenta_background) },
        { "light-cyan-background",    sgr::call(sgr::bright_cyan_background) },
        { "light-white-background",   sgr::call(sgr::bright_white_background) }
    };
    for (const auto& [in, out]: mapping) {
        if (string.find(in, offset) == offset) {
            return { out, in.size() };
        }
    }
    return {};
}

}

namespace frog::log {

static std::string_view file_in_path(std::string_view path) {
    if (const auto slash = path.rfind(std::filesystem::path::preferred_separator); slash != std::string::npos) {
        return path.substr(slash + 1);
    } else {
        return path;
    }
}

static std::string current_local_time_string() {
    const auto now = std::time(nullptr);
    tm local_time{};
    localtime_r(&now, &local_time);
    char buffer[64]{};
    std::strftime(buffer, 64, "%X", &local_time);
    return buffer;
}

void print(const source_with_format& format, std::string_view type, std::string message) {
    if (type == "notice") {
        message = "%light-blue" + message;
    } else if (type == "warning") {
        message = "%yellow" + message;
    } else if (type == "error") {
        message = "%red" + message;
    }
    std::size_t offset{ 0 };
    auto begin = message.find('%', offset);
    while (begin != std::string::npos) {
        offset = begin + 1;
        if (const auto& [as_ansi, replace_size] = ansi::format_ansi_text_graphics(message, offset); !as_ansi.empty()) {
            message.replace(message.begin() + begin, message.begin() + begin + replace_size + 1, as_ansi);
        }
        begin = message.find('%', offset);
    }
    message += ansi::sgr::call(ansi::sgr::reset);

    const auto timeMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    const auto timeString = current_local_time_string() + "." + std::to_string(timeMs % 1000);
    const auto filename = file_in_path(format.source.file_name());

    std::osyncstream out{ std::cout };
    out << std::left
        << std::setw(13)
        << timeString
        << std::setw(1)
        << std::internal
        << filename
        << ": "
        << std::setw(4)
        << format.source.line()
        << ": "
        << std::setw(1)
        << type
        << ": "
        << message
        << "\n";
}

}
