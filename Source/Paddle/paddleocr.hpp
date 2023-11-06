#pragma once

#include "PaddleTextOrientationClassifier.hpp"
#include "PaddleTextDetector.hpp"
#include "Recognition.hpp"
#include "Config.hpp"

namespace frog {

struct PPOCR {

    PPOCR(bool cls, bool rec, bool use_angle_cls);

    std::vector<OCRPredictResult> ocr(cv::Mat image, bool rec, bool cls) const;
    void recognize(std::vector<cv::Mat> img_list, std::vector<OCRPredictResult>& ocr_results) const;
    void classify(std::vector<cv::Mat> img_list, std::vector<OCRPredictResult>& ocr_results) const;

};

}
