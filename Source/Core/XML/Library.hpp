#pragma once

#include <functional>
#include <filesystem>
#include <optional>

namespace frog::xml::library {

void initialize();
void cleanup();
void resolveExternalResourceToLocalFile(std::function<std::optional<std::filesystem::path>(std::string_view)> resolver);

}
