#pragma once

#include "Core/Timer.hpp"
#include "Core/Formatting.hpp"
#include "Core/XML/Document.hpp"
#include "OCR/Engine.hpp"
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

namespace frog::application::about {
constexpr std::string_view creator{ "Norsk helsearkiv" };
constexpr std::string_view name{ "FrogOCR" };
constexpr std::string_view version{ "1.5.2" };
constexpr std::string_view build_date{ __DATE__ };
}

namespace frog::application {

void start();

std::vector<Task> fetch_next_tasks(const database::Connection& database, int count);
void do_task(const Task& task, ocr::Engine& engine);

std::filesystem::path launch_path();
std::stack<std::string_view> launch_arguments();
std::string version_with_build_date();

}
