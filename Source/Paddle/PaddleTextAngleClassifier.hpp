#pragma once

#include "paddle_api.h"
#include "paddle_inference_api.h"
#include "utility.hpp"
#include "Image.hpp"

namespace frog {

struct PaddleTextAngleClassifierConfig;

struct Classification {
    unsigned int label{};
    float confidence{};

    int angle() const {
        return label % 2 == 1 ? 180 : 0;
    }
};

struct PaddleTextAngleClassifier {

    PaddleTextAngleClassifier(const PaddleTextAngleClassifierConfig& config);

    std::vector<Classification> classify(PIX* image, const std::vector<Quad>& quads);

private:

    std::shared_ptr<paddle_infer::Predictor> predictor;

};

}
