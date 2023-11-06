#pragma once

#include "paddle_api.h"
#include "paddle_inference_api.h"

#include "Image.hpp"
#include "TextDetection.hpp"
#include "Config.hpp"
#include "opencv2/imgproc.hpp"

namespace frog {

class PaddleTextDetector : public TextDetector {
public:

    PaddleTextDetector(const PaddleTextDetectorConfig& config);

    std::vector<Quad> detect(const Image& image, const TextDetectionSettings& settings) override;

private:

    std::shared_ptr<paddle_infer::Predictor> predictor;

};

}
