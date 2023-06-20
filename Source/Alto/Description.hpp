#pragma once

#include "MeasurementUnit.hpp"
#include "SourceImageInformation.hpp"
#include "Processing.hpp"

namespace frog::alto {

struct Description {

    MeasurementUnit measurementUnit;
    SourceImageInformation sourceImageInformation;
    std::vector<Processing> processings;

};

}
