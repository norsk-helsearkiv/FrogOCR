#pragma once

#include "paddleocr.hpp"
#include "Postprocessing.hpp"
#include "Preprocessing.hpp"

#include "paddle_api.h"
#include "paddle_inference_api.h"

namespace frog {

class StructureTableRecognizer {
public:

    int table_max_len_{ 488 };
    std::vector<float> mean_{ 0.485f, 0.456f, 0.406f };
    std::vector<float> scale_{ 1 / 0.229f, 1 / 0.224f, 1 / 0.225f };
    bool is_scale_{ true };
    int table_batch_num_{ 1 };

    StructureTableRecognizer(std::string model_dir, std::string label_path);

    void LoadModel(const std::string& model_dir);
    void Run(std::vector<cv::Mat> img_list, std::vector<std::vector<std::string>>& rec_html_tags, std::vector<float>& rec_scores, std::vector<std::vector<std::vector<int>>>& rec_boxes);

private:

    std::shared_ptr<paddle_infer::Predictor> predictor_;
    TablePostProcessor post_processor_;

};

class StructureLayoutRecognizer {
public:

    std::vector<float> mean_{ 0.485f, 0.456f, 0.406f };
    std::vector<float> scale_{ 1 / 0.229f, 1 / 0.224f, 1 / 0.225f };
    bool is_scale_ = true;

    StructureLayoutRecognizer(std::string model_dir, std::string label_path);

    void LoadModel(const std::string& model_dir);
    void Run(cv::Mat img, std::vector<StructurePredictResult>& result);

private:

    std::shared_ptr<paddle_infer::Predictor> predictor_;
    PicodetPostProcessor post_processor_;

};

class PaddleStructure {
public:

    PaddleStructure(bool layout, bool table);

    std::vector<StructurePredictResult> structure(cv::Mat img, bool layout = false, bool table = true, bool ocr = false);

private:

    std::unique_ptr<StructureTableRecognizer> table_model_;
    std::unique_ptr<StructureLayoutRecognizer> layout_model_;

    void layout(cv::Mat img, std::vector<StructurePredictResult>& structure_result);
    void table(cv::Mat img, StructurePredictResult& structure_result);
    std::string rebuild_table(std::vector<std::string> rec_html_tags, std::vector<std::vector<int>> rec_boxes, std::vector<OCRPredictResult>& ocr_result);

    float dis(std::vector<int>& box1, std::vector<int>& box2);

};

}
