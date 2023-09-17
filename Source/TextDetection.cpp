#include "TextDetection.hpp"
#include "Core/Formatting.hpp"
#include "Core/Buffer.hpp"

namespace frog {

PIX* copy_pixels_in_quad(PIX* source, const Quad& quad) {
    const auto top = static_cast<int>(quad.top());
    const auto bottom = static_cast<int>(quad.bottom());
    const auto left = static_cast<int>(quad.left());
    const auto right = static_cast<int>(quad.right());
    auto destination = pixCreate(right - left, bottom - top, static_cast<l_int32>(source->d));
    pixSetBlackOrWhite(destination, L_SET_WHITE);
    for (int y{ top }; y <= bottom; y++) {
        for (int x{ left }; x <= right; x++) {
            if (quad.contains(static_cast<float>(x), static_cast<float>(y))) {
                l_uint32 pixel{};
                pixGetPixel(source, x, y, &pixel);
                pixSetPixel(destination, x - left, y - top, pixel);
            }
        }
    }
    return destination;
}

std::vector<Quad> detect_text_quads(const std::filesystem::path& python, const std::filesystem::path& script, const std::filesystem::path& path) {
    const auto command = fmt::format("{} {} \"{}\"", python, script, path);
    Buffer stream;
    if (auto process = popen(reinterpret_cast<const char*>(command.c_str()), "r")) {
        std::size_t lastReadSize{};
        bool tryAgain{};
        stream.allocate(32768);
        do {
            tryAgain = false;
            lastReadSize = std::fread(stream.at_write(), 1, stream.size_left_to_write(), process);
            if (lastReadSize != 0) {
                stream.move_write_index(static_cast<long long>(lastReadSize));
                stream.resize_if_needed(2048);
            } else {
                if (std::ferror(process) != 0) {
                    stream.reset();
                } else if (std::feof(process) == 0) {
                    tryAgain = true;
                }
            }
        } while (lastReadSize != 0 || tryAgain);
        pclose(process);
        *stream.write_position = '\0';
        const std::string_view allOutput{ stream.begin, stream.size_left_to_read() };
        const auto beginMarkerIndex = allOutput.find("BEGIN PADDLE FROG");
        if (beginMarkerIndex == std::string_view::npos) {
            return {};
        }
        const auto output = allOutput.substr(beginMarkerIndex + 15);
        std::vector<Quad> quads;
        for (const auto& boxLines : split_string_view(output, "\n")) {
            fmt::print("{}\n", boxLines);
            const auto coord = split_string_view(boxLines, " ");
            if (coord.empty()) {
                continue;
            }
            if (coord.size() < 8) {
                fmt::print("Unexpected output from text detection script: {}", boxLines);
                continue;
            }
            Quad quad;
            quad.x1 = from_string<float>(coord[0]).value_or(0.0f);
            quad.y1 = from_string<float>(coord[1]).value_or(0.0f);
            quad.x2 = from_string<float>(coord[2]).value_or(0.0f);
            quad.y2 = from_string<float>(coord[3]).value_or(0.0f);
            quad.x3 = from_string<float>(coord[4]).value_or(0.0f);
            quad.y3 = from_string<float>(coord[5]).value_or(0.0f);
            quad.x4 = from_string<float>(coord[6]).value_or(0.0f);
            quad.y4 = from_string<float>(coord[7]).value_or(0.0f);
            quads.push_back(quad);
        }
        return quads;
    } else {
        fmt::print("Failed: {}\n", errno);
    }
    return {};
}

}
