#pragma once

#include <ostream>

namespace frog::ocr {

enum class PageSegmentation {
    automatic,
    sparse,
    block,
    word,
    line
};

}

std::ostream& operator<<(std::ostream& out, frog::ocr::PageSegmentation pageSegmentation);
