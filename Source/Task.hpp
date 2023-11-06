#pragma once

#include <string>

namespace frog {

struct Task {
    std::int64_t taskId{};
    std::string inputPath;
    std::string outputPath;
    std::string settingsCsv; // setting=value CSV
};

}
