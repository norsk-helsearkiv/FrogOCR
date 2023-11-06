#pragma once

#include "Core/Quad.hpp"
#include "Document.hpp"
#include "Settings.hpp"

#include <vector>

namespace frog {

class Image;

class TextRecognizer {
public:

    TextRecognizer() = default;
    TextRecognizer(const TextRecognizer&) = delete;
    TextRecognizer(TextRecognizer&&) = delete;

    virtual ~TextRecognizer() = default;

    TextRecognizer& operator=(const TextRecognizer&) = delete;
    TextRecognizer& operator=(TextRecognizer&&) = delete;

    virtual Document recognize(const Image& image, const std::vector<Quad>& quads, const std::vector<int>& angles, const TextRecognitionSettings& settings) = 0;

};

}
