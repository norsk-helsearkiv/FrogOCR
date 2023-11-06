#pragma once

#include "paddle_api.h"
#include "paddle_inference_api.h"

#include "PaddleTextOrientationClassifier.hpp"
#include "utility.hpp"

namespace frog {

struct PaddleRecognizer {

    PaddleRecognizer(std::string model_directory, std::string label_path);

    void LoadModel(std::string model_directory);
    void Run(std::vector<cv::Mat> img_list, std::vector<std::string>& rec_texts, std::vector<float>& rec_text_scores);

private:

    std::shared_ptr<paddle_infer::Predictor> predictor;
    std::vector<std::string> labels;

};

}
