#pragma once

#include <chrono>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <cstdlib>
#include <vector>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <numeric>

#include "Core/Quad.hpp"
#include "opencv2/core.hpp"
#include "opencv2/imgcodecs.hpp"

namespace frog {

struct OCRPredictResult {
    std::vector<std::vector<int>> box;
    std::string text;
    float score = -1.0f;
    float cls_score;
    int cls_label = -1;
};

struct StructurePredictResult {
    std::vector<float> box;
    std::vector<std::vector<int>> cell_box;
    std::string type;
    std::vector<OCRPredictResult> text_res;
    std::string html;
    float html_score = -1.0f;
    float confidence;
};

class Utility {
public:

    static std::vector<std::string> ReadDict(const std::string& path);

    template<class ForwardIterator>
    static size_t argmax(ForwardIterator first, ForwardIterator last) {
        return std::distance(first, std::max_element(first, last));
    }

    static void print_result(const std::vector<OCRPredictResult>& ocr_result);

    static cv::Mat crop_image(cv::Mat& img, const std::vector<int>& area);
    static cv::Mat crop_image(cv::Mat& img, const std::vector<float>& area);

    static std::vector<int> xyxyxyxy2xyxy(std::vector<std::vector<int>>& box);
    static std::vector<int> xyxyxyxy2xyxy(std::vector<int>& box);

    static float fast_exp(float x);
    static std::vector<float> activation_function_softmax(std::vector<float>& src);
    static float iou(std::vector<int>& box1, std::vector<int>& box2);
    static float iou(std::vector<float>& box1, std::vector<float>& box2);

};

}
