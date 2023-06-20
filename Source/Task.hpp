#pragma once

#include <string>

namespace frog::application {

struct Task {
    std::int64_t taskId{};
    std::string inputPath;
    std::string outputPath;
    std::string settings; // setting=value CSV
};

}
