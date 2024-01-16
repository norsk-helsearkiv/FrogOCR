#pragma once

#include "Core/Quad.hpp"
#include "Document.hpp"
#include "Settings.hpp"

#include <leptonica/allheaders.h>

#include <vector>

namespace frog {

class TextRecognizer {
public:

    TextRecognizer() = default;
    TextRecognizer(const TextRecognizer&) = delete;
    TextRecognizer(TextRecognizer&&) = delete;

    virtual ~TextRecognizer() = default;

    TextRecognizer& operator=(const TextRecognizer&) = delete;
    TextRecognizer& operator=(TextRecognizer&&) = delete;

    virtual Document recognize(PIX* image, const std::vector<Quad>& quads, std::vector<int> angles, const TextRecognitionSettings& settings) const = 0;

};

}
