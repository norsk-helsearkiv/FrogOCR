#pragma once

#include "Core/XML/Node.hpp"

#include <string>

namespace frog::alto {

struct SourceImageInformation {

    std::filesystem::path fileName;

    SourceImageInformation() = default;

    SourceImageInformation(std::filesystem::path fileName_) : fileName{ std::move(fileName_) } {
    }

};

}
