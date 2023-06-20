#pragma once

#include "Core/XML/Node.hpp"

#include <string>

namespace frog::alto {

struct TextStyle {

    static constexpr int defaultFontSize{ 10 };

    std::string fontFamily;
    int fontSize{ defaultFontSize };

    TextStyle() = default;

    TextStyle(std::string fontFamily, int fontSize) : fontFamily{ std::move(fontFamily) }, fontSize{ fontSize } {
    }

};

}
