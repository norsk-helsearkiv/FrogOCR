#pragma once

#include "ProcessingSoftware.hpp"

#include <vector>

namespace frog::alto {

enum class ProcessingCategory {
    content_generation,
    content_modification,
    pre_operation,
    post_operation,
    other
};

struct Processing {
    std::optional<ProcessingCategory> processingCategory;
    std::string processingDateTime;
    std::string processingAgency;
    std::string processingStepDescription;
    std::vector<std::string> processingStepSettings;
    ProcessingSoftware processingSoftware;
};

}
