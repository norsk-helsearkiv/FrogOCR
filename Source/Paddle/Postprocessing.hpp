#pragma once

#include "Core/Quad.hpp"
#include "clipper.hpp"
#include "utility.hpp"

namespace frog {

class TablePostProcessor {
public:

    void init(std::string label_path, bool merge_no_span_structure = true);
    void Run(std::vector<float>& loc_preds, std::vector<float>& structure_probs,
             std::vector<float>& rec_scores, std::vector<int>& loc_preds_shape,
             std::vector<int>& structure_probs_shape,
             std::vector<std::vector<std::string>>& rec_html_tag_batch,
             std::vector<std::vector<std::vector<int>>>& rec_boxes_batch,
             std::vector<int>& width_list, std::vector<int>& height_list);

private:

    std::vector<std::string> label_list_;
    std::string end = "eos";
    std::string beg = "sos";

};

class PicodetPostProcessor {
public:

    void init(std::string label_path, double score_threshold = 0.4, double nms_threshold = 0.5, const std::vector<int>& fpn_stride = { 8, 16, 32, 64 });
    void Run(std::vector<StructurePredictResult>& results, std::vector<std::vector<float>> outs, std::vector<int> ori_shape, std::vector<int> resize_shape, int eg_max);
    std::vector<int> fpn_stride_{ 8, 16, 32, 64 };

private:

    StructurePredictResult disPred2Bbox(std::vector<float> bbox_pred, int label, float score, int x, int y, int stride, std::vector<int> im_shape, int reg_max);
    void nms(std::vector<StructurePredictResult>& input_boxes, float nms_threshold);

    std::vector<std::string> label_list_;
    double score_threshold_ = 0.4;
    double nms_threshold_ = 0.5;
    int num_class_ = 5;
};

}
