#pragma once

#include "Block.hpp"
#include "Core/Log.hpp"

namespace frog::ocr {

struct Document {
    std::vector<std::pair<std::string, int>> fonts;
    std::vector<Block> blocks;
    Confidence confidence;

    Document() = default;
    Document(Document&&) = default;
    Document(const Document&) = delete;

    Document& operator=(Document&&) = default;
    Document& operator=(const Document&) = delete;
};

struct PerformanceProfile {
    // Loading the image and initializing settings.
    long long loadMs{};

    // Preparing the OCR document structure.
    long long overheadMs{};

    // Time spent in Tesseract.
    long long ocrMs{};
};

}
