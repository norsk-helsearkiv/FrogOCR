#pragma once

#include "ComposedBlock.hpp"

namespace frog::alto {

class Page;

struct PrintSpace {

    int hpos{};
    int vpos{};
    int width{};
    int height{};
    std::vector<ComposedBlock> composedBlocks;

    PrintSpace() = default;

    PrintSpace(int hpos, int vpos, int width, int height) : hpos{ hpos }, vpos{ vpos }, width{ width }, height{ height } {
    }

    std::vector<const String*> getStringsInArea(int x, int y, int width, int height) const;
    TextLine* findClosestTextLine(int x, int y, int width, int height, int required_min_vertical_distance);

};

}
