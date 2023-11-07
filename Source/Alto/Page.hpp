#pragma once

#include "PrintSpace.hpp"

namespace frog::alto {

struct Page {

    std::string language;
    int width{};
    int height{};
    int physicalImageNumber{};
    float confidence{};
    float rotationInDegrees{};
    std::vector<PrintSpace> printSpaces;

    Page() = default;

    Page(std::string language_, int width, int height, int physicalImageNumber, float confidence, float rotationInDegrees)
        : language{ std::move(language_) }, width{ width }, height{ height }, physicalImageNumber{ physicalImageNumber }, confidence{ confidence }, rotationInDegrees{ rotationInDegrees } {
    }

};

}
