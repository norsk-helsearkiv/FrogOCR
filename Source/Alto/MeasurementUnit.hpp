#pragma once

#include "Core/XML/Node.hpp"
#include "Core/Formatting.hpp"

namespace frog::alto {

struct MeasurementUnit {

    static constexpr std::string_view pixel{ "pixel" };
    static constexpr std::string_view mm10{ "mm10" }; // 1/10th of a millimeter
    static constexpr std::string_view inch1200{ "inch1200" }; // 1/1200th of an inch

    std::string type{ pixel };

    MeasurementUnit() = default;

    MeasurementUnit(std::string type_) : type{ std::move(type_) } {
    }

};

}
