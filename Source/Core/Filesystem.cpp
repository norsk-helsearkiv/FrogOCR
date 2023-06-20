#include "Filesystem.hpp"

#include <fstream>

namespace frog {

template<typename DirectoryIterator>
static std::vector<std::filesystem::path> iterate_entries_in_directory(const std::filesystem::path& path, entry_inclusion inclusion, const std::function<bool(const std::filesystem::path&)>& predicate, std::size_t preallocated) {
	thread_local std::vector<std::filesystem::path> entries;
	entries.clear();
	if (preallocated > entries.capacity()) {
		entries.reserve(preallocated);
	}
	std::error_code begin_error;
	DirectoryIterator entry{ path, std::filesystem::directory_options::skip_permission_denied, begin_error };
	while (entry != std::filesystem::end(entry)) {
		const bool skip{ inclusion != entry_inclusion::everything && ((entry->is_directory() && inclusion == entry_inclusion::only_files) || (!entry->is_directory() && inclusion == entry_inclusion::only_directories)) };
		if (!skip && (!predicate || predicate(*entry))) {
			entries.push_back(*entry);
		}
		auto entry_backup = entry;
		std::error_code increment_error;
		entry.increment(increment_error);
		if constexpr (std::is_same_v<DirectoryIterator, std::filesystem::recursive_directory_iterator>) {
			while (increment_error && entry_backup.depth() != 0) {
				entry = entry_backup;
				std::error_code pop_error;
				entry.pop(pop_error);
				if (pop_error == std::error_code{} && entry != std::filesystem::end(entry)) {
					entry_backup = entry;
					entry.increment(increment_error);
				}
			}
		}
	}
	return entries;
}

#ifdef WIN32
std::filesystem::path _workaround_fix_windows_path(std::filesystem::path path) {
	// todo: this is a visual c++ bug, so check if this is fixed later.
	if (path.u8string().back() == ':') {
		path /= "/";
	} else {
		auto path_string = path.u8string();
		if (const auto index = path_string.find(':'); index != std::string::npos) {
			if (index + 1 < path_string.size()) {
				if (path_string[index + 1] != '/' && path_string[index + 1] != '\\') {
					path_string.insert(path_string.begin() + index + 1, '/');
					path = path_string;
				}
			}
		}
	}
	return path;
}
#endif

std::vector<std::filesystem::path> entries_in_directory(std::filesystem::path path, entry_inclusion inclusion, bool recursive, const std::function<bool(const std::filesystem::path&)>& predicate, std::size_t preallocated) {
	if (path.empty()) {
		return {};
	}
#ifdef WIN32
	path = _workaround_fix_windows_path(path);
#endif
	if (recursive) {
		return iterate_entries_in_directory<std::filesystem::recursive_directory_iterator>(path, inclusion, predicate, preallocated);
	} else {
		return iterate_entries_in_directory<std::filesystem::directory_iterator>(path, inclusion, predicate, preallocated);
	}
}

std::vector<std::filesystem::path> files_in_directory(std::filesystem::path path, bool recursive, std::optional<std::string_view> extension, std::size_t preallocated) {
	return entries_in_directory(path, entry_inclusion::only_files, true, extension ? [extension](const std::filesystem::path& path) {
		return path.extension() == extension.value();
	} : std::function<bool(const std::filesystem::path&)>{}, preallocated);
}

bool write_file(const std::filesystem::path& path, std::string_view source) {
	std::error_code error;
	std::filesystem::create_directories(path.parent_path(), error);
    if (error) {
        return false;
    }
	if (std::ofstream file{ path, std::ios::binary }; file.is_open()) {
		file << source;
        file.close();
        return !file.bad();
	} else {
        return false;
    }
}

bool write_file(const std::filesystem::path& path, const char* source, std::streamsize size) {
	std::error_code error;
	std::filesystem::create_directories(path.parent_path(), error);
	if (std::ofstream file{ path, std::ios::binary }; file.is_open()) {
		file.write(source, size);
        file.close();
        return !file.bad();
	} else {
        return false;
    }
}

bool append_file(const std::filesystem::path& path, std::string_view source) {
	if (std::ofstream file{ path, std::ios::app }; file.is_open()) {
		file << source;
        file.close();
        return !file.bad();
	} else {
        return false;
    }
}

std::string read_file(const std::filesystem::path& path) {
	if (const std::ifstream file{ path, std::ios::binary }) {
		std::stringstream result;
		result << file.rdbuf();
		return result.str();
	} else {
		return {};
	}
}

}
