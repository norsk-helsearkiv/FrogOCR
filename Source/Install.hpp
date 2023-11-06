#pragma once

#include <string>
#include <stack>

namespace frog {

struct DatabaseConfig;
struct Config;

void create_database(const DatabaseConfig& config);
void cli_config(std::stack<std::string_view> arguments);
void cli_install(std::stack<std::string_view> arguments, const Config& config);

}
