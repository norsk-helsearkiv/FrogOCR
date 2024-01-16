#pragma once

#include "TextDetection.hpp"

namespace frog {

class IntegratedTextDetector : public TextDetector {
public:

    std::vector<Quad> detect(PIX* image, const TextDetectionSettings& settings) const override;

};

}
