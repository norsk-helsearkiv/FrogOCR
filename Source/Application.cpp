#include "Application.hpp"
#include "TaskProcessor.hpp"
#include "Core/XML/Library.hpp"
#include "Image.hpp"
#include "Core/Filesystem.hpp"
#include "Core/Log.hpp"
#include "Core/Database/Connection.hpp"
#include "Core/XML/Validator.hpp"
#include "Install.hpp"

#include <csignal>

namespace frog {

static bool running{ true };

void signal_handler(int signal) {
    running = false;
}

std::filesystem::path launch_path() {
    if (const auto& args = launch_arguments(); !args.empty()) {
        return std::filesystem::path{ args.top() }.parent_path();
    } else {
        return std::filesystem::current_path();
    }
}

std::optional<std::filesystem::path> find_configuration_path() {
    const std::vector<std::filesystem::path> candidates{
        "/etc/frog/config.xml",
        "Resources/config.xml",
        "config.xml",
        "../../Resources/config.xml"
    };
    for (const auto& candidate: candidates) {
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }
    return std::nullopt;
}

std::string version_with_build_date() {
    return fmt::format("{} (build date: {})", about::version, about::build_date);
}

std::string get_tesseract_version() {
    return tesseract::TessBaseAPI::Version();
}

std::string get_paddle_version() {
    // TODO: Figure out why get_version() returns 0.0.0... It seems to be a CMake issue.
    return "2.5.1";
}

std::string get_opencv_version() {
    return cv::getVersionString();
}

void delete_task(const database::Connection& database, std::int64_t taskId) {
    database.execute(R"(delete from task where task_id = $1)", { std::to_string(taskId) });
}

void add_task(const database::Connection& database, std::string_view inputPath, std::string_view outputPath, std::int32_t priority, std::string_view customData1, std::int64_t customData2, std::string_view settings) {
    database.execute(R"(
        insert into task (input_path, output_path, priority, custom_data_1, custom_data_2, settings_csv)
             values ($1, $2, $3, $4, $5, $6)
    )", {
        inputPath, outputPath, std::to_string(priority), customData1, std::to_string(customData2), settings
    });
}

void show_versions() {
    fmt::print("Frog {} (build date: {})\n", about::version, about::build_date);
    fmt::print("Tesseract {}\n", get_tesseract_version());
    fmt::print("Paddle {}\n", get_paddle_version());
    fmt::print("OpenCV {}\n", get_opencv_version());
}

void show_command_help(std::string_view command) {
    if (command == "add") {
        fmt::print("add <path> [--database <index>] [--output <path>] [--recursive] [--custom-data-1 <string>] [--custom-data-2 <int64>]\n");
    } else if (command == "process") {
        fmt::print("process [--input <path>] [--recursive] [--exit-if-no-tasks]\n");
        fmt::print("  --exit-if-no-tasks  Exit instead of sleeping when there are no tasks to process\n");
    } else if (command == "validate") {
        fmt::print("validate <path>\n");
    } else if (command == "install") {
        fmt::print("install (--configure | --create-database)\n\n");
        fmt::print("  --configure         Install default configuration\n");
        fmt::print("  --create-database   Sets up configured databases\n");
    } else {
        fmt::print("Unknown command: {}\n", command);
    }
}

void show_help() {
    fmt::print("usage: frog [--help [<command>] | --version] <command> [<args>]\n\n");
    fmt::print("Frog converts images to text with Tesseract and Paddle.\n\n");
    fmt::print("COMMANDS\n");
    fmt::print("  add         Insert new tasks into the queue\n");
    fmt::print("  process     Process tasks\n");
    fmt::print("  validate    Validate files according to schema\n");
    fmt::print("  install     Configure and setup\n");
    fmt::print("OPTIONS\n");
    fmt::print("  -h, --help  Show this information\n");
    fmt::print("  --version   Show application and library versions\n");
    fmt::print("\n");
}

void cli_help(std::stack<std::string_view> arguments) {
    if (arguments.empty()) {
        show_help();
    } else {
        show_command_help(arguments.top());
    }
}

void cli_add(std::stack<std::string_view> arguments, const Config& config) {
    if (arguments.empty()) {
        fmt::print("Path to file or directory is required.\n");
        return;
    }
    int addTasksDatabaseIndex{};
    auto addTasksPath = arguments.top();
    arguments.pop();
    std::optional<std::filesystem::path> addTasksOutputPath;
    std::string newTasksCustomData1;
    std::int64_t newTasksCustomData2{};
    int priority{};
    Settings settings;
    while (!arguments.empty()) {
        const auto argument = arguments.top();
        arguments.pop();
        if (argument == "--output") {
            if (!arguments.empty()) {
                addTasksOutputPath = arguments.top();
                arguments.pop();
            }
        } else if (argument == "--database") {
            if (!arguments.empty()) {
                addTasksDatabaseIndex = from_string<int>(arguments.top()).value_or(0);
                arguments.pop();
            } else {
                fmt::print("No database specified with --database.\n");
            }
        } else if (argument == "--custom-data-1") {
            if (!arguments.empty()) {
                newTasksCustomData1 = arguments.top();
                arguments.pop();
            } else {
                fmt::print("No string specified with --custom-data-1.\n");
            }
        } else if (argument == "--custom-data-2") {
            if (!arguments.empty()) {
                if (const auto maybeCustomData2 = from_string<std::int64_t>(arguments.top())) {
                    newTasksCustomData2 = maybeCustomData2.value();
                } else {
                    fmt::print("Invalid integer specified with --custom-data-2.\n");
                }
                arguments.pop();
            } else {
                fmt::print("No integer specified with --custom-data-2.\n");
            }
        } else if (argument == "--setting") {
            if (arguments.size() >= 2) {
                const auto key = arguments.top();
                arguments.pop();
                const auto value = arguments.top();
                arguments.pop();
                settings.set(key, value);
            } else {
                fmt::print("No setting given with --setting.\n");
            }
        } else if (argument == "--priority") {
            if (!arguments.empty()) {
                if (const auto maybePriority = from_string<int>(arguments.top())) {
                    priority = maybePriority.value();
                } else {
                    fmt::print("Invalid integer specified with --priority.\n");
                }
                arguments.pop();
            } else {
                fmt::print("No integer specified with --priority.\n");
            }
        }
    }

    fmt::print("Add tasks from path: {}", addTasksPath);
    std::vector<std::pair<std::filesystem::path, std::filesystem::path>> newTaskPaths;
    const auto inputDirectoryString = path_to_string(addTasksPath);
    if (std::filesystem::is_directory(addTasksPath)) {
        const auto& inputPaths = files_in_directory(addTasksPath, true, ".jpg");
        newTaskPaths.reserve(inputPaths.size());
        for (const auto& inputPath : inputPaths) {
            if (addTasksOutputPath.has_value()) {
                const auto inputPathString = path_to_string(inputPath);
                auto relativeInputPathString = inputPathString.substr(inputDirectoryString.size());
                while (relativeInputPathString.starts_with('/')) {
                    relativeInputPathString.erase(0, 1);
                }
                newTaskPaths.emplace_back(inputPath, addTasksOutputPath.value() / path_with_extension(relativeInputPathString, "xml"));
            } else {
                newTaskPaths.emplace_back(inputPath, path_with_extension(inputPath, "xml"));
            }
        }
    } else if (std::filesystem::is_regular_file(addTasksPath)) {
        if (addTasksOutputPath.has_value()) {
            newTaskPaths.emplace_back(addTasksPath, addTasksOutputPath.value());
        } else {
            newTaskPaths.emplace_back(addTasksPath, path_with_extension(addTasksPath, "xml"));
        }
    } else {
        log::error("No directory or file found at specified path.");
        return;
    }
    if (addTasksDatabaseIndex >= static_cast<int>(config.databases.size())) {
        log::error("Database not configured: {}. There are {} databases configured.", addTasksDatabaseIndex, config.databases.size());
        return;
    }
    const auto& databaseConfig = config.databases[addTasksDatabaseIndex];
    const database::Connection database{ databaseConfig.host, databaseConfig.port, databaseConfig.name, databaseConfig.username, databaseConfig.password };
    const auto& settingsCsv = settings.csv();
    for (const auto& [inputPath, outputPath] : newTaskPaths) {
        const auto inputPathString = path_to_string(inputPath);
        const auto outputPathString = path_to_string(outputPath);
        add_task(database, inputPathString, outputPathString, priority, newTasksCustomData1, newTasksCustomData2, settingsCsv);
    }
}

void cli_process(std::stack<std::string_view> arguments, const Config& config) {
    bool exitIfNoTasks{ false };
    while (!arguments.empty()) {
        const auto argument = arguments.top();
        arguments.pop();
        if (argument == "--exit-if-no-tasks") {
            exitIfNoTasks = true;
        }
    }
    if (config.profiles.empty()) {
        fmt::print("No profiles are configured.\n");
        return;
    }
    const auto& profile = config.profiles.front();

    std::vector<std::unique_ptr<TaskProcessor>> processors;
    for (int i{ 0 }; i < config.maxThreadCount; i++) {
        processors.emplace_back(std::make_unique<TaskProcessor>(profile));
    }
    log::info("Initialized {} task processors", processors.size());

    while (running) {
        std::vector<std::unique_ptr<database::Connection>> databaseConnections;
        for (const auto& databaseConfig : config.databases) {
            auto connection = std::make_unique<database::Connection>(databaseConfig.host, databaseConfig.port, databaseConfig.name, databaseConfig.username, databaseConfig.password);
            if (connection->has_error()) {
                log::warning("Failed to connect to database {} at {}", databaseConfig.name, databaseConfig.host);
            } else {
                databaseConnections.emplace_back(std::move(connection));
            }
        }
        if (databaseConnections.empty()) {
            log::warning("Failed to connect to database. Trying again in 5 minutes.");
            std::this_thread::sleep_for(std::chrono::minutes{ 5 });
            continue;
        }
        std::vector<Task> tasks;
        for (const auto& databaseConnection : databaseConnections) {
            tasks = fetch_next_tasks(*databaseConnection, config.maxThreadCount * 100);
            if (!tasks.empty()) {
                break;
            }
        }
        if (tasks.empty()) {
            if (exitIfNoTasks) {
                log::info("No tasks in queue. Exiting.");
                running = false;
            } else {
                log::info("No tasks in queue. Checking again in 30 seconds.");
                std::this_thread::sleep_for(std::chrono::seconds{ 30 });
            }
        }
        while (!tasks.empty()) {
            for (auto& processor: processors) {
                if (processor->getRemainingTaskCount() > 100) {
                    continue;
                }
                processor->pushTask(tasks.back());
                if (processor->isFinished()) {
                    processor->relaunch();
                }
                tasks.pop_back();
                if (tasks.empty()) {
                    break;
                }
            }
            if (!tasks.empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds{ 500 });
            }
        }
    }
}

void cli_validate(std::stack<std::string_view> arguments, const Config& config) {
    if (arguments.empty()) {
        fmt::print("Path to a file or directory is required.\n");
        return;
    }
    auto validatePath = arguments.top();
    arguments.pop();

    log::info("Validate path: {}", validatePath);
    std::vector<std::filesystem::path> validatePaths;
    if (std::filesystem::is_directory(validatePath)) {
        validatePaths = files_in_directory(validatePath, true, ".xml");
    } else if (std::filesystem::is_regular_file(validatePath)) {
        validatePaths = { validatePath };
    } else {
        log::error("No directory or file found at specified path.");
        return;
    }
    const xml::Validator validator{ config.schemas / "alto.xsd" };
    const auto pathCount = validatePaths.size();
    std::size_t pathIndex{};
    for (const auto& path: validatePaths) {
        pathIndex++;
        const auto xml = read_file(path);
        fmt::print("[{:>6}/{}] Validating {} [{:>5} KiB] ... ", pathIndex, pathCount, path, xml.size() / 1024);
        if (xml.empty()) {
            fmt::print("EMPTY\n");
            continue;
        }
        if (const auto status = validator.validate(xml); status.has_value()) {
            fmt::print("ERROR - {}\n", status.value());
        } else {
            fmt::print("OK\n");
        }
    }
}

void start() {
    auto arguments = launch_arguments();
    if (arguments.empty()) {
        show_help();
        return;
    }
    const auto launchPath = arguments.top();
    arguments.pop();
    if (arguments.empty()) {
        show_help();
        return;
    }
    const auto commandName = arguments.top();
    arguments.pop();

    if (commandName == "--version") {
        show_versions();
        return;
    }

    if (commandName == "help" || commandName == "--help" || commandName == "-h") {
        cli_help(arguments);
        return;
    }

    std::signal(SIGTERM, signal_handler);

    const auto configurationXmlPath = find_configuration_path();
    if (!configurationXmlPath.has_value()) {
        log::error("Unable to locate configuration");
        return;
    }
    xml::library::initialize();
    Config config{ xml::Document{ read_file(configurationXmlPath.value()) } };

    if (commandName == "config") {
        cli_config(arguments);
        return;
    }

    if (!std::filesystem::exists(config.schemas)) {
        log::error("Did not find schema directory: {}", config.schemas);
        return;
    }

    xml::library::resolveExternalResourceToLocalFile([&config](std::string_view url) -> std::optional<std::filesystem::path> {
        if (url == "http://www.loc.gov/standards/alto/alto-4-4.xsd" || url == "alto-4-4.xsd") {
            return config.schemas / "alto-4-4.xsd";
        } else if (url == "http://www.loc.gov/standards/xlink/xlink.xsd") {
            return config.schemas / "xlink.xsd";
        } else {
            return std::nullopt;
        }
    });

    if (config.maxThreadCount == 0) {
        config.maxThreadCount = static_cast<int>(std::thread::hardware_concurrency());
        if (config.maxThreadCount == 0) {
            config.maxThreadCount = 1;
            log::warning("Failed to detect number of processors. Please configure manually. Defaulting to 1.");
        } else {
            log::info("Setting max thread count based on detected number of processors: %cyan{}", config.maxThreadCount);
        }
    }

    if (commandName == "add") {
        cli_add(arguments, config);
        return;
    }

    if (commandName == "validate") {
        cli_validate(arguments, config);
        return;
    }

    if (commandName == "process") {
        cli_process(arguments, config);
        return;
    }
}

std::vector<Task> fetch_next_tasks(const database::Connection& database, int count) {
    log::info("Fetching next {} tasks", count);
    const auto result = database.execute(fmt::format(R"(
           select task_id,
                  input_path,
                  output_path,
                  settings_csv
             from task
         order by priority desc
            limit {}
    )", count));
    std::vector<Task> tasks;
    for (int i{ 0 }; i < result.count(); i++) {
        const auto row = result.row(i);
        Task task;
        task.taskId = row.long_integer("task_id");
        task.inputPath = row.text("input_path");
        task.outputPath = row.text("output_path");
        task.settingsCsv = row.text("settings_csv");
        tasks.emplace_back(std::move(task));
    }
    for (const auto& task: tasks) {
        delete_task(database, task.taskId);
    }
    return tasks;
}

}
