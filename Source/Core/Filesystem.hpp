#pragma once

#include <filesystem>
#include <functional>
#include <optional>

namespace frog {

enum class entry_inclusion { everything, only_files, only_directories };

std::vector<std::filesystem::path> entries_in_directory(std::filesystem::path path, entry_inclusion inclusion, bool recursive, const std::function<bool(const std::filesystem::path&)>& predicate, std::size_t preallocated = 0);
std::vector<std::filesystem::path> files_in_directory(std::filesystem::path path, bool recursive, std::optional<std::string_view> extension = std::nullopt, std::size_t preallocated = 0);
bool write_file(const std::filesystem::path& path, std::string_view source);
bool write_file(const std::filesystem::path& path, const char* source, std::streamsize size);
bool append_file(const std::filesystem::path& path, std::string_view source);
std::string read_file(const std::filesystem::path& path);

}
