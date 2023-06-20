#pragma once

#include "Core/XML/Node.hpp"

namespace frog::alto {

struct Variant {

    std::string content;
    float confidence{};

    Variant() = default;

    Variant(std::string_view content, float confidence) : content{ content }, confidence{ confidence } {
    }

};

}
