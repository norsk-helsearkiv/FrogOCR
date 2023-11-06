#include "Postprocessing.hpp"

#include <ranges>

namespace frog {

void TablePostProcessor::init(std::string label_path, bool merge_no_span_structure) {
    label_list_ = Utility::ReadDict(label_path);
    if (merge_no_span_structure) {
        label_list_.emplace_back("<td></td>");
        std::vector<std::string>::iterator it;
        for (it = label_list_.begin(); it != label_list_.end();) {
            if (*it == "<td>") {
                it = label_list_.erase(it);
            } else {
                ++it;
            }
        }
    }
    label_list_.insert(label_list_.begin(), beg);
    label_list_.push_back(end);
}

void TablePostProcessor::Run(std::vector<float>& loc_preds, std::vector<float>& structure_probs, std::vector<float>& rec_scores, std::vector<int>& loc_preds_shape, std::vector<int>& structure_probs_shape, std::vector<std::vector<std::string>>& rec_html_tag_batch, std::vector<std::vector<std::vector<int>>>& rec_boxes_batch, std::vector<int>& width_list, std::vector<int>& height_list) {
    for (int batch_idx = 0; batch_idx < structure_probs_shape[0]; batch_idx++) {
        // image tags and boxs
        std::vector<std::string> rec_html_tags;
        std::vector<std::vector<int>> rec_boxes;

        float score = 0.f;
        int count = 0;
        float char_score = 0.f;
        int char_idx = 0;

        // step
        for (int step_idx = 0; step_idx < structure_probs_shape[1]; step_idx++) {
            std::string html_tag;
            std::vector<int> rec_box;
            // html tag
            int step_start_idx = (batch_idx * structure_probs_shape[1] + step_idx) * structure_probs_shape[2];
            char_idx = int(Utility::argmax(&structure_probs[step_start_idx], &structure_probs[step_start_idx + structure_probs_shape[2]]));
            char_score = float(*std::max_element(&structure_probs[step_start_idx], &structure_probs[step_start_idx + structure_probs_shape[2]]));
            html_tag = this->label_list_[char_idx];

            if (step_idx > 0 && html_tag == this->end) {
                break;
            }
            if (html_tag == this->beg) {
                continue;
            }
            count += 1;
            score += char_score;
            rec_html_tags.push_back(html_tag);

            // box
            if (html_tag == "<td>" || html_tag == "<td" || html_tag == "<td></td>") {
                for (int point_idx = 0; point_idx < loc_preds_shape[2]; point_idx++) {
                    step_start_idx = (batch_idx * structure_probs_shape[1] + step_idx) * loc_preds_shape[2] + point_idx;
                    float point = loc_preds[step_start_idx];
                    if (point_idx % 2 == 0) {
                        point = int(point * width_list[batch_idx]);
                    } else {
                        point = int(point * height_list[batch_idx]);
                    }
                    rec_box.push_back(point);
                }
                rec_boxes.push_back(rec_box);
            }
        }
        score /= count;
        if (std::isnan(score) || rec_boxes.size() == 0) {
            score = -1;
        }
        rec_scores.push_back(score);
        rec_boxes_batch.push_back(rec_boxes);
        rec_html_tag_batch.push_back(rec_html_tags);
    }
}

void PicodetPostProcessor::init(std::string label_path, double score_threshold, double nms_threshold, const std::vector<int>& fpn_stride) {
    this->label_list_ = Utility::ReadDict(label_path);
    this->score_threshold_ = score_threshold;
    this->nms_threshold_ = nms_threshold;
    this->num_class_ = label_list_.size();
    this->fpn_stride_ = fpn_stride;
}

void PicodetPostProcessor::Run(std::vector<StructurePredictResult>& results, std::vector<std::vector<float>> outs, std::vector<int> ori_shape, std::vector<int> resize_shape, int reg_max) {
    int in_h = resize_shape[0];
    int in_w = resize_shape[1];
    float scale_factor_h = resize_shape[0] / float(ori_shape[0]);
    float scale_factor_w = resize_shape[1] / float(ori_shape[1]);

    std::vector<std::vector<StructurePredictResult>> bbox_results;
    bbox_results.resize(this->num_class_);
    for (int i = 0; i < this->fpn_stride_.size(); ++i) {
        int feature_h = std::ceil((float) in_h / this->fpn_stride_[i]);
        int feature_w = std::ceil((float) in_w / this->fpn_stride_[i]);
        for (int idx = 0; idx < feature_h * feature_w; idx++) {
            // score and label
            float score = 0;
            int cur_label = 0;
            for (int label = 0; label < this->num_class_; label++) {
                if (outs[i][idx * this->num_class_ + label] > score) {
                    score = outs[i][idx * this->num_class_ + label];
                    cur_label = label;
                }
            }
            // bbox
            if (score > this->score_threshold_) {
                int row = idx / feature_w;
                int col = idx % feature_w;
                std::vector<float> bbox_pred(outs[i + this->fpn_stride_.size()].begin() + idx * 4 * reg_max, outs[i + this->fpn_stride_.size()].begin() + (idx + 1) * 4 * reg_max);
                bbox_results[cur_label].push_back(this->disPred2Bbox(bbox_pred, cur_label, score, col, row, this->fpn_stride_[i], resize_shape, reg_max));
            }
        }
    }
    for (int i = 0; i < bbox_results.size(); i++) {
        if (bbox_results[i].empty()) {
            continue;
        }
        this->nms(bbox_results[i], this->nms_threshold_);
        for (auto box : bbox_results[i]) {
            box.box[0] = box.box[0] / scale_factor_w;
            box.box[2] = box.box[2] / scale_factor_w;
            box.box[1] = box.box[1] / scale_factor_h;
            box.box[3] = box.box[3] / scale_factor_h;
            results.push_back(box);
        }
    }
}

StructurePredictResult PicodetPostProcessor::disPred2Bbox(std::vector<float> bbox_pred, int label, float score, int x, int y, int stride, std::vector<int> im_shape, int reg_max) {
    float ct_x = (x + 0.5) * stride;
    float ct_y = (y + 0.5) * stride;
    std::vector<float> dis_pred;
    dis_pred.resize(4);
    for (int i = 0; i < 4; i++) {
        float dis = 0;
        std::vector<float> bbox_pred_i(bbox_pred.begin() + i * reg_max, bbox_pred.begin() + (i + 1) * reg_max);
        std::vector<float> dis_after_sm = Utility::activation_function_softmax(bbox_pred_i);
        for (int j = 0; j < reg_max; j++) {
            dis += j * dis_after_sm[j];
        }
        dis *= stride;
        dis_pred[i] = dis;
    }

    float xmin = (std::max)(ct_x - dis_pred[0], .0f);
    float ymin = (std::max)(ct_y - dis_pred[1], .0f);
    float xmax = (std::min)(ct_x + dis_pred[2], (float) im_shape[1]);
    float ymax = (std::min)(ct_y + dis_pred[3], (float) im_shape[0]);

    StructurePredictResult result_item;
    result_item.box = { xmin, ymin, xmax, ymax };
    result_item.type = this->label_list_[label];
    result_item.confidence = score;
    return result_item;
}

void PicodetPostProcessor::nms(std::vector<StructurePredictResult>& input_boxes, float nms_threshold) {
    std::sort(input_boxes.begin(), input_boxes.end(), [](StructurePredictResult a, StructurePredictResult b) {
        return a.confidence > b.confidence;
    });
    std::vector<int> picked(input_boxes.size(), 1);
    for (int i = 0; i < input_boxes.size(); ++i) {
        if (picked[i] == 0) {
            continue;
        }
        for (int j = i + 1; j < input_boxes.size(); ++j) {
            if (picked[j] == 0) {
                continue;
            }
            float iou = Utility::iou(input_boxes[i].box, input_boxes[j].box);
            if (iou > nms_threshold) {
                picked[j] = 0;
            }
        }
    }
    std::vector<StructurePredictResult> input_boxes_nms;
    for (int i = 0; i < input_boxes.size(); ++i) {
        if (picked[i] == 1) {
            input_boxes_nms.push_back(input_boxes[i]);
        }
    }
    input_boxes = input_boxes_nms;
}

}
