#include "Install.hpp"
#include "Core/Log.hpp"
#include "Core/Database/Connection.hpp"
#include "Config.hpp"

namespace frog {

constexpr std::string_view defaultConfiguration{
    R"(<?xml version="1.0" encoding="UTF-8"?>)"
    "\n"
    "<Configuration>\n"
    "\t<MaxThreadCount>0</MaxThreadCount>\n"
    "\t<Schemas>/etc/frog/schemas</Schemas>\n"
    "\t<Database>\n"
    "\t\t<Host>localhost</Host>\n"
    "\t\t<Port>5432</Port>\n"
    "\t\t<Name>frog</Name>\n"
    "\t\t<Username>frog</Username>\n"
    "\t\t<Password>frog</Password>\n"
    "\t</Database>\n"
    "\t<Tesseract>\n"
    "\t\t<Tessdata>/etc/frog/tessdata</Tessdata>\n"
    "\t\t<Dataset>nor</Dataset>\n"
    "\t</Tesseract>\n"
    "</Configuration>\n"
};

void create_database(const DatabaseConfig& config) {
    const database::Connection database{ config.host, config.port, config.name, config.username, config.password };
    database.execute(R"(
        create type log_level as enum (
            'debug', 'info', 'notice', 'warning', 'error', 'critical', 'alert', 'emergency'
        );
    )");
    database.execute(R"(
        create table log (
            log_id      bigserial not null primary key,
            level       log_level not null,
            message     text      not null,
            created_at  timestamp not null default current_timestamp
        );
    )");
    database.execute(R"(
        create table task (
            task_id        bigserial not null primary key,
            input_path     text      not null,
            output_path    text      not null,
            priority       int       not null default 0,
            custom_data_1  text          null default null,
            custom_data_2  bigint        null default null,
            settings_csv   text          null default null,
            created_at     timestamp not null default current_timestamp,

            constraint unique_task_input_path  unique (input_path),
            constraint unique_task_output_path unique (output_path)
        );
    )");
    database.execute(R"(create index index_task_custom_data_1 on task (custom_data_1);)");
    database.execute(R"(create index index_task_custom_data_2 on task (custom_data_2);)");
}

void cli_config(std::stack<std::string_view> arguments) {
    std::error_code errorCode;
    if (!std::filesystem::is_directory("/etc/frog")) {
        if (!std::filesystem::create_directory("/etc/frog", errorCode)) {
            log::error("Failed to create directory: %cyan/etc/frog");
            return;
        }
    }
    if (!std::filesystem::exists("/etc/frog/config.xml")) {
        if (!write_file("/etc/frog/config.xml", defaultConfiguration)) {
            log::error("Failed to create file %cyan/etc/frog/config.xml");
        }
    }
}

void cli_install(std::stack<std::string_view> arguments, const Config& config) {
    while (!arguments.empty()) {
        const auto argument = arguments.top();
        if (argument == "--create-database") {
            for (const auto& databaseConfig : config.databases) {
                create_database(databaseConfig);
            }
        }
    }
}

}
