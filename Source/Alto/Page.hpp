#pragma once

#include "PrintSpace.hpp"

namespace frog::alto {

struct Page {

    int width{};
    int height{};
    int physicalImageNumber{};
    float confidence{};
    std::vector<PrintSpace> printSpaces;

    Page() = default;

    Page(int width, int height, int physicalImageNumber, float confidence) : width{ width }, height{ height }, physicalImageNumber{ physicalImageNumber }, confidence{ confidence } {
    }

};

}
