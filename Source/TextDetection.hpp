#pragma once

#include "Core/Formatting.hpp"
#include "Core/Quad.hpp"
#include "PreProcess/Image.hpp"

#include <filesystem>
#include <vector>

namespace frog {

PIX* copy_pixels_in_quad(PIX* source, const Quad& quad);
std::vector<Quad> detect_text_quads(const std::filesystem::path& python, const std::filesystem::path& script, const std::filesystem::path& path);

}
