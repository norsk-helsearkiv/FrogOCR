#pragma once

#include "Core/Quad.hpp"
#include "Document.hpp"
#include "Settings.hpp"

#include <leptonica/allheaders.h>

#include <vector>

namespace frog {

class TextDetector {
public:

    TextDetector() = default;
    TextDetector(const TextDetector&) = delete;
    TextDetector(TextDetector&&) = delete;

    virtual ~TextDetector() = default;

    TextDetector& operator=(const TextDetector&) = delete;
    TextDetector& operator=(TextDetector&&) = delete;

    virtual std::vector<Quad> detect(PIX* image, const TextDetectionSettings& settings) const = 0;

};

}
