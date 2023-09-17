#include "Application.hpp"
#include "TaskProcessor.hpp"
#include "Configuration.hpp"
#include "Core/XML/Library.hpp"
#include "OCR/Scan.hpp"
#include "OCR/Settings.hpp"
#include "OCR/Dataset.hpp"
#include "PreProcess/Image.hpp"
#include "Core/Filesystem.hpp"
#include "Core/Log.hpp"
#include "Core/Database/Connection.hpp"
#include "Alto/Alto.hpp"
#include "Alto/WriteXml.hpp"
#include "Core/XML/Validator.hpp"
#include "PostProcess/Tidy.hpp"
#include "TextDetection.hpp"

#include <csignal>

namespace frog::application {

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
        "/etc/Frog/Configuration.xml",
        "Resources/Configuration.xml",
        "Configuration.xml",
        "../../Resources/Configuration.xml"
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

void delete_task(const database::Connection& database, std::int64_t taskId) {
    database.execute(R"(delete from "Task" where "TaskId" = $1)", { std::to_string(taskId) });
}

void add_task(const database::Connection& database, std::string_view inputPath, std::string_view outputPath, std::int32_t priority, std::string_view customData1, std::int64_t customData2, std::string_view settings) {
    database.execute(R"(
        insert into "Task" ("InputPath", "OutputPath", "Priority", "CustomData1", "CustomData2", "Settings")
             values ($1, $2, $3, $4, $5, $6)
    )", {
        inputPath, outputPath, std::to_string(priority), customData1, std::to_string(customData2), settings
    });
}

void start() {
    bool showHelp{ false };
    bool showVersion{ false };
    bool createDatabase{ false };
    bool install{ false };
    bool validate{ false };
    bool exitIfNoTasks{ false };
    bool exitBeforeWork{ false };
    std::optional<std::filesystem::path> validatePath;
    std::optional<std::filesystem::path> addTasksPath;
    std::optional<std::filesystem::path> addTasksOutputPath;
    std::string newTasksCustomData1;
    std::int64_t newTasksCustomData2{};
    std::string textDetectorName{ "Tesseract" };

    auto arguments = launch_arguments();
    while (!arguments.empty()) {
        const auto argument = arguments.top();
        arguments.pop();
        if (argument == "--version") {
            showVersion = true;
            continue;
        }
        if (argument == "--help") {
            showHelp = true;
            continue;
        }
        if (argument == "--add-tasks") {
            if (!arguments.empty()) {
                addTasksPath = arguments.top();
                arguments.pop();
            }
            continue;
        }
        if (argument == "--validate") {
            validate = true;
            if (!arguments.empty()) {
                validatePath = arguments.top();
                arguments.pop();
            }
            continue;
        }
        if (argument == "--output") {
            if (!arguments.empty()) {
                addTasksOutputPath = arguments.top();
                arguments.pop();
            }
            continue;
        }
        if (argument == "--text-detector") {
            if (!arguments.empty()) {
                textDetectorName = arguments.top();
                arguments.pop();
            } else {
                log::warning("No text detector specified with --text-detector.");
            }
            continue;
        }
        if (argument == "--custom-data-1") {
            if (!arguments.empty()) {
                newTasksCustomData1 = arguments.top();
                arguments.pop();
            } else {
                log::warning("No string specified with --custom--data-1.");
            }
            continue;
        }
        if (argument == "--custom-data-2") {
            if (!arguments.empty()) {
                newTasksCustomData2 = from_string<std::int64_t>(arguments.top()).value_or(0);
                arguments.pop();
            } else {
                log::warning("No integer specified with --custom--data-2.");
            }
            continue;
        }
        if (argument == "--exit-if-no-tasks") {
            exitIfNoTasks = true;
            continue;
        }
        if (argument == "--exit-before-work") {
            exitBeforeWork = true;
            continue;
        }
        if (argument == "--create-database") {
            createDatabase = true;
            continue;
        }
        if (argument == "--install") {
            install = true;
            continue;
        }
    }

    if (showVersion) {
        fmt::print("FrogOCR {} (build date: {})\n", about::version, about::build_date);
        fmt::print("Tesseract {}\n", tesseract::TessBaseAPI::Version());
        return;
    }

    if (showHelp) {
        fmt::print("FrogOCR converts images to text using Tesseract.\n");
        fmt::print("--version                Prints the FrogOCR and Tesseract versions\n");
        fmt::print("--help                   Prints this information\n");
        fmt::print("--add-tasks [path]       Adds tasks recursively from given path\n");
        fmt::print("--validate [path]        Validate AltoXML files\n");
        fmt::print("--text-detector [string] Tesseract (Default) or Paddle\n");
        fmt::print("--output [path]          If used with --add-tasks, files are written to the specified directory\n");
        fmt::print("--custom-data-1 [string] If used with --add-tasks, sets CustomData1 string\n");
        fmt::print("--custom-data-2 [int64]  If used with --add-tasks, sets CustomData2 64-bit integer\n");
        fmt::print("--exit-if-no-tasks       Exit instead of sleeping when there are no tasks to process\n");
        fmt::print("--exit-before-work       Useful if --add-tasks is used and you don't want to process them yet\n");
        fmt::print("--install                Install default configuration\n");
        fmt::print("--create-database        Creates the Task table in the configured database\n");
        return;
    }

    if (install) {
        std::error_code errorCode;
        if (!std::filesystem::is_directory("/etc/Frog")) {
            if (!std::filesystem::create_directory("/etc/Frog", errorCode)) {
                log::error("Failed to create directory: %cyan/etc/Frog");
                return;
            }
        }
        if (!std::filesystem::exists("/etc/Frog/Configuration.xml")) {
            constexpr std::string_view defaultConfiguration{
                R"(<?xml version="1.0" encoding="UTF-8"?>)"
                "\n"
                "<Configuration>\n"
                "\t<MaxThreadCount>0</MaxThreadCount>\n"
                "\t<Tessdata>/etc/Frog/tessdata</Tessdata>\n"
                "\t<Schemas>/etc/Frog/Schemas</Schemas>\n"
                "\t<DefaultDataset>nor</DefaultDataset>\n"
                "\t<Database>\n"
                "\t\t<Host>localhost</Host>\n"
                "\t\t<Port>5432</Port>\n"
                "\t\t<Name>frog</Name>\n"
                "\t\t<Username>frog</Username>\n"
                "\t\t<Password>frog</Password>\n"
                "\t</Database>\n"
                "\t<Python>/opt/conda/envs/paddle-frog/bin/python</Python>\n"
                "\t<PaddleFrog>/opt/paddle-frog.py</PaddleFrog>\n"
                "</Configuration>\n"
            };
            if (!write_file("/etc/Frog/Configuration.xml", defaultConfiguration)) {
                log::error("Failed to create file %cyan/etc/Frog/Configuration.xml");
            }
        }
        return;
    }

    const auto configurationXmlPath = find_configuration_path();
    if (!configurationXmlPath.has_value()) {
        log::error("Unable to locate configuration");
    }
    xml::library::initialize();
    Configuration configuration{ xml::Document{ read_file(configurationXmlPath.value()) }};

    if (createDatabase) {
        constexpr std::string_view createTaskTableSql{
            "create table \"Task\" ("
            "    \"TaskId\"      bigserial not null primary key,"
            "    \"InputPath\"   text      not null,"
            "    \"OutputPath\"  text      not null,"
            "    \"Priority\"    int       not null default 0,"
            "    \"CustomData1\" text      null     default null,"
            "    \"CustomData2\" bigint    null     default null,"
            "    \"Settings\"    text      null     default null,"
            "    \"CreatedAt\"   timestamp not null default current_timestamp,"
            "    constraint \"Unique_Task_InputPath\"  unique (\"InputPath\"),"
            "    constraint \"Unique_Task_OutputPath\" unique (\"OutputPath\")"
            ");"
        };
        constexpr std::string_view createTaskCustomData1Index{
            R"(create index "Index_Task_CustomData1" on "Task" ("CustomData1");)"
        };
        constexpr std::string_view createTaskCustomData2Index{
            R"(create index "Index_Task_CustomData2" on "Task" ("CustomData2");)"
        };
        database::Connection database{
            configuration.database.host,
            configuration.database.port,
            configuration.database.name,
            configuration.database.username,
            configuration.database.password
        };
        database.execute(createTaskTableSql);
        database.execute(createTaskCustomData1Index);
        database.execute(createTaskCustomData2Index);
        return;
    }

    if (!std::filesystem::exists(configuration.tessdataPath)) {
        log::error("Did not find tessdata directory: {}", configuration.tessdataPath);
        return;
    }

    if (!std::filesystem::exists(configuration.schemasPath)) {
        log::error("Did not find schema directory: {}", configuration.schemasPath);
        return;
    }

    xml::library::resolveExternalResourceToLocalFile([&configuration](std::string_view url) -> std::optional<std::filesystem::path> {
        if (url == "http://www.loc.gov/standards/alto/alto-4-2.xsd" || url == "alto-4-2.xsd") {
            return configuration.schemasPath / "alto-4-2.xsd";
        } else if (url == "http://www.loc.gov/standards/xlink/xlink.xsd") {
            return configuration.schemasPath / "xlink.xsd";
        } else {
            return std::nullopt;
        }
    });

    // Finish configuration
    if (configuration.maxThreadCount == 0) {
        configuration.maxThreadCount = static_cast<int>(std::thread::hardware_concurrency());
        if (configuration.maxThreadCount == 0) {
            configuration.maxThreadCount = 1;
            log::warning("Failed to detect number of processors. Please configure manually. Defaulting to 1.");
        } else {
            log::info("Setting max thread count based on detected number of processors: %cyan{}", configuration.maxThreadCount);
        }
    }

    // Validate
    if (validate) {
        if (!validatePath.has_value()) {
            log::error("File to validate not specified.");
            return;
        }
        log::info("Validate path: {}", validatePath.value());
        std::vector<std::filesystem::path> validatePaths;
        if (std::filesystem::is_directory(validatePath.value())) {
            validatePaths = files_in_directory(validatePath.value(), true, ".xml");
        } else if (std::filesystem::is_regular_file(validatePath.value())) {
            validatePaths = { validatePath.value() };
        } else {
            log::error("No directory or file found at specified path.");
            return;
        }
        const xml::Validator validator{ configuration.schemasPath / "alto.xsd" };
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
        return;
    }

    // Add tasks
    if (addTasksPath.has_value()) {
        log::info("Add tasks from path: {}", addTasksPath.value());
        std::vector<std::pair<std::filesystem::path, std::filesystem::path>> newTaskPaths;
        const auto inputDirectoryString = path_to_string(addTasksPath.value());
        if (std::filesystem::is_directory(addTasksPath.value())) {
            const auto& inputPaths = files_in_directory(addTasksPath.value(), true, ".jpg");
            newTaskPaths.reserve(inputPaths.size());
            for (const auto& inputPath : inputPaths) {
                if (addTasksOutputPath.has_value()) {
                    const auto inputPathString = path_to_string(inputPath);
                    auto relativeInputPathString = inputPathString.substr(inputDirectoryString.size());
                    while (relativeInputPathString.starts_with('/')) {
                        relativeInputPathString.erase(0, 1);
                    }
                    auto outputPath = addTasksOutputPath.value() / path_with_extension(relativeInputPathString, "xml");
                    newTaskPaths.emplace_back(inputPath, outputPath);
                } else {
                    newTaskPaths.emplace_back(inputPath, path_with_extension(inputPath, "xml"));
                }
            }
        } else if (std::filesystem::is_regular_file(addTasksPath.value())) {
            if (addTasksOutputPath.has_value()) {
                const auto inputPathString = path_to_string(addTasksPath.value());
                auto relativeInputPathString = inputPathString.substr(inputDirectoryString.size());
                while (relativeInputPathString.starts_with('/')) {
                    relativeInputPathString.erase(0, 1);
                }
                auto outputPath = addTasksOutputPath.value() / path_with_extension(relativeInputPathString, "xml");
                newTaskPaths.emplace_back(addTasksPath.value(), outputPath);
            } else {
                newTaskPaths.emplace_back(addTasksPath.value(), path_with_extension(addTasksPath.value(), "xml"));
            }
        } else {
            log::error("No directory or file found at specified path.");
            return;
        }
        const database::Connection database{
            configuration.database.host,
            configuration.database.port,
            configuration.database.name,
            configuration.database.username,
            configuration.database.password
        };
        std::string_view pageSegmentation{ "Block" };
        if (textDetectorName == "Paddle") {
            pageSegmentation = "Line";
        }
        const auto settings = fmt::format("SauvolaKFactor=0.16,TextDetector={},PageSegmentation={}", textDetectorName, pageSegmentation);
        for (const auto& [inputPath, outputPath] : newTaskPaths) {
            const auto inputPathString = path_to_string(inputPath);
            const auto outputPathString = path_to_string(outputPath);
            add_task(database, inputPathString, outputPathString, 0, newTasksCustomData1, newTasksCustomData2, settings);
        }
    }

    // Sometimes you just want to relax.
    if (exitBeforeWork) {
        return;
    }

    // Load datasets
    ocr::Dataset dataset{ configuration.defaultDataset, configuration.tessdataPath };

    // Initialize all engines
    std::vector<std::unique_ptr<TaskProcessor>> processors;
    for (int i{ 1 }; i <= configuration.maxThreadCount; i++) {
        auto engine = std::make_unique<ocr::Engine>(dataset);
        if (!engine->isOk()) {
            log::error("Engine #{} failed to initialize.", i);
            break;
        }
        processors.emplace_back(std::make_unique<TaskProcessor>(std::move(engine)));
    }
    log::info("Initialized {} engines", processors.size());

    std::signal(SIGTERM, signal_handler);

    while (running) {
        const database::Connection database{
            configuration.database.host,
            configuration.database.port,
            configuration.database.name,
            configuration.database.username,
            configuration.database.password
        };
        if (database.has_error()) {
            log::warning("Failed to connect to database. Trying again in 5 minutes.");
            std::this_thread::sleep_for(std::chrono::minutes{ 5 });
            continue;
        }

        auto tasks = fetch_next_tasks(database, configuration.maxThreadCount * 100);
        if (tasks.empty()) {
            if (exitIfNoTasks) {
                log::info("No tasks in queue. Exiting.");
                running = false;
            } else {
                log::info("No tasks in queue. Checking again in 10 minutes.");
                std::this_thread::sleep_for(std::chrono::minutes{ 10 });
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
    log::info("Exited successfully.");
}

std::vector<Task> fetch_next_tasks(const database::Connection& database, int count) {
    log::info("Fetching next {} tasks", count);
    const auto result = database.execute(fmt::format(R"(
           select "TaskId",
                  "InputPath",
                  "OutputPath",
                  "Settings"
             from "Task"
         order by "Priority" desc
            limit {}
    )", count));
    std::vector<Task> tasks;
    for (int i{ 0 }; i < result.count(); i++) {
        const auto row = result.row(i);
        Task task;
        task.taskId = row.long_integer("\"TaskId\"");
        task.inputPath = row.text("\"InputPath\"");
        task.outputPath = row.text("\"OutputPath\"");
        task.settings = row.text("\"Settings\"");
        tasks.emplace_back(std::move(task));
    }
    for (const auto& task: tasks) {
        delete_task(database, task.taskId);
    }
    return tasks;
}

void do_task(const Task& task, ocr::Engine& engine) {
    // Pre-checks
    if (std::filesystem::exists(task.outputPath)) {
        log::warning("Output file already exists: %cyan{}", task.outputPath);
        return;
    }
    if (!std::filesystem::exists(task.inputPath)) {
        log::error("Input file does not exist: %cyan{}", task.inputPath);
        return;
    }

    // Initialize
    preprocess::Image image{ task.inputPath };
    if (!image.isOk()) {
        log::error("Failed to load image: %cyan{}", task.inputPath);
        return;
    }
    const ocr::Settings settings{ task.settings };

    // Perform OCR
    auto ocrDocument = ocr::scan(engine, image, settings);

    // Post-process OCR
    postprocess::remove_garbage(ocrDocument);

    // Create Alto
    alto::Alto alto{ ocrDocument, image, settings };
    if (!write_file(task.outputPath, alto::to_xml(alto))) {
        log::error("Failed to write AltoXML file: {}", task.outputPath);
    }
}

}
