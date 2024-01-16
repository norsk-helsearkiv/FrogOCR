#pragma once

#include <string>

namespace frog {

struct Task {
    std::int64_t taskId{};
    std::string inputPath;
    std::string outputPath;
    std::string customData1;
    std::int64_t customData2{};
    std::string settingsCsv; // setting=value CSV
};

}
