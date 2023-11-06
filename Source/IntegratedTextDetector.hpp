#pragma once

#include "TextDetection.hpp"

namespace frog {

class IntegratedTextDetector : public TextDetector {
public:

    std::vector<Quad> detect(const Image& image, const TextDetectionSettings& settings) override;

};

}
