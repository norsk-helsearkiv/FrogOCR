#pragma once

#include "Core/Timer.hpp"
#include "Core/Formatting.hpp"
#include "Core/XML/Document.hpp"
#include "Task.hpp"

#include <tesseract/baseapi.h>

#include <optional>
#include <vector>
#include <filesystem>
#include <stack>
#include <thread>
#include <memory>
#include <mutex>

namespace frog::database {
class Connection;
}

namespace frog::about {
constexpr std::string_view creator{ "Norsk helsearkiv" };
constexpr std::string_view name{ "Frog" };
constexpr std::string_view version{ "1.7.0" };
constexpr std::string_view build_date{ __DATE__ };
}

namespace frog {

struct Config;

void start();

std::vector<Task> fetch_next_tasks(const database::Connection& database, int count);

std::filesystem::path launch_path();
std::stack<std::string_view> launch_arguments();
std::string version_with_build_date();
std::string get_tesseract_version();
std::string get_paddle_version();
std::string get_opencv_version();

}
