#pragma once

#include "paddle_api.h"
#include "paddle_inference_api.h"
#include "utility.hpp"

namespace frog {

struct PaddleTextOrientationClassifierConfig;
class Image;

struct Classification {
    unsigned int label{};
    float confidence{};

    int angle() const {
        return label % 2 == 1 ? 180 : 0;
    }
};

struct PaddleTextOrientationClassifier {

    PaddleTextOrientationClassifier(const PaddleTextOrientationClassifierConfig& config);

    std::vector<Classification> classify(const Image& image, const std::vector<Quad>& quads);

private:

    std::shared_ptr<paddle_infer::Predictor> predictor;

};

}
