#include "Structure.hpp"
#include "Preprocessing.hpp"
#include "Postprocessing.hpp"

namespace frog {

void StructureLayoutRecognizer::Run(cv::Mat img, std::vector<StructurePredictResult>& result) {
    // preprocess
    auto preprocess_start = std::chrono::steady_clock::now();
    cv::Mat srcimg;
    img.copyTo(srcimg);
    cv::Mat resize_img;
    resize(srcimg, resize_img, 800, 608);
    normalize(&resize_img, mean_, scale_, is_scale_);
    std::vector<float> input(1 * 3 * resize_img.rows * resize_img.cols, 0.0f);
    permute_rgb_to_chw(resize_img, input.data());

    // inference.
    auto input_names = predictor_->GetInputNames();
    auto input_t = predictor_->GetInputHandle(input_names[0]);
    input_t->Reshape({ 1, 3, resize_img.rows, resize_img.cols });
    auto inference_start = std::chrono::steady_clock::now();
    input_t->CopyFromCpu(input.data());
    predictor_->Run();

    // Get output tensor
    std::vector<std::vector<float>> out_tensor_list;
    std::vector<std::vector<int>> output_shape_list;
    auto output_names = predictor_->GetOutputNames();
    for (int j = 0; j < output_names.size(); j++) {
        auto output_tensor = predictor_->GetOutputHandle(output_names[j]);
        std::vector<int> output_shape = output_tensor->shape();
        int out_num = std::accumulate(output_shape.begin(), output_shape.end(), 1, std::multiplies<int>());
        output_shape_list.push_back(output_shape);
        std::vector<float> out_data;
        out_data.resize(out_num);
        output_tensor->CopyToCpu(out_data.data());
        out_tensor_list.push_back(out_data);
    }

    // postprocess
    std::vector<int> bbox_num;
    int reg_max = 0;
    for (int i = 0; i < out_tensor_list.size(); i++) {
        if (i == post_processor_.fpn_stride_.size()) {
            reg_max = output_shape_list[i][2] / 4;
            break;
        }
    }
    std::vector<int> ori_shape{ srcimg.rows, srcimg.cols };
    std::vector<int> resize_shape{ resize_img.rows, resize_img.cols };
    post_processor_.Run(result, out_tensor_list, ori_shape, resize_shape, reg_max);
    bbox_num.push_back(result.size());
}

StructureLayoutRecognizer::StructureLayoutRecognizer(std::string model_dir, std::string label_path) {
    post_processor_.init(label_path, 0.5f, 0.5f);
    LoadModel(model_dir);
}

void StructureLayoutRecognizer::LoadModel(const std::string& model_dir) {
    paddle_infer::Config config;
    config.SetModel(model_dir + "/inference.pdmodel", model_dir + "/inference.pdiparams");
    config.DisableGpu();
    config.EnableMKLDNN();
    config.SetCpuMathLibraryNumThreads(1);
    config.SwitchUseFeedFetchOps(false);
    config.SwitchSpecifyInputNames(true);
    config.SwitchIrOptim(true);
    config.EnableMemoryOptim();
#ifdef NDEBUG
    config.DisableGlogInfo();
#endif
    predictor_ = paddle_infer::CreatePredictor(config);
}

void StructureTableRecognizer::Run(std::vector<cv::Mat> img_list, std::vector<std::vector<std::string>>& structure_html_tags, std::vector<float>& structure_scores, std::vector<std::vector<std::vector<int>>>& structure_boxes) {
    int img_num = img_list.size();
    for (int beg_img_no = 0; beg_img_no < img_num; beg_img_no += table_batch_num_) {
        // preprocess
        int end_img_no = std::min(img_num, beg_img_no + table_batch_num_);
        int batch_num = end_img_no - beg_img_no;
        std::vector<cv::Mat> norm_img_batch;
        std::vector<int> width_list;
        std::vector<int> height_list;
        for (int ino = beg_img_no; ino < end_img_no; ino++) {
            cv::Mat srcimg;
            img_list[ino].copyTo(srcimg);
            cv::Mat resize_img;
            cv::Mat pad_img;
            table_resize_image(srcimg, resize_img, table_max_len_);
            normalize(&resize_img, mean_, scale_, is_scale_);
            table_pad_image(resize_img, pad_img, table_max_len_);
            norm_img_batch.push_back(pad_img);
            width_list.push_back(srcimg.cols);
            height_list.push_back(srcimg.rows);
        }

        std::vector<float> input(batch_num * 3 * table_max_len_ * table_max_len_, 0.0f);
        permute_rgb_to_chw_batch(norm_img_batch, input.data());
        // inference.
        auto input_names = predictor_->GetInputNames();
        auto input_t = predictor_->GetInputHandle(input_names[0]);
        input_t->Reshape({ batch_num, 3, table_max_len_, table_max_len_ });
        auto inference_start = std::chrono::steady_clock::now();
        input_t->CopyFromCpu(input.data());
        predictor_->Run();
        auto output_names = predictor_->GetOutputNames();
        auto output_tensor0 = predictor_->GetOutputHandle(output_names[0]);
        auto output_tensor1 = predictor_->GetOutputHandle(output_names[1]);
        std::vector<int> predict_shape0 = output_tensor0->shape();
        std::vector<int> predict_shape1 = output_tensor1->shape();

        int out_num0 = std::accumulate(predict_shape0.begin(), predict_shape0.end(), 1, std::multiplies<int>());
        int out_num1 = std::accumulate(predict_shape1.begin(), predict_shape1.end(), 1, std::multiplies<int>());
        std::vector<float> loc_preds;
        std::vector<float> structure_probs;
        loc_preds.resize(out_num0);
        structure_probs.resize(out_num1);

        output_tensor0->CopyToCpu(loc_preds.data());
        output_tensor1->CopyToCpu(structure_probs.data());
        // postprocess
        std::vector<std::vector<std::string>> structure_html_tag_batch;
        std::vector<float> structure_score_batch;
        std::vector<std::vector<std::vector<int>>> structure_boxes_batch;
        post_processor_.Run(loc_preds, structure_probs, structure_score_batch, predict_shape0, predict_shape1, structure_html_tag_batch, structure_boxes_batch, width_list, height_list);
        for (int m = 0; m < predict_shape0[0]; m++) {
            structure_html_tag_batch[m].emplace(structure_html_tag_batch[m].begin(), "<table>");
            structure_html_tag_batch[m].emplace(structure_html_tag_batch[m].begin(), "<body>");
            structure_html_tag_batch[m].emplace(structure_html_tag_batch[m].begin(), "<html>");
            structure_html_tag_batch[m].emplace_back("</table>");
            structure_html_tag_batch[m].emplace_back("</body>");
            structure_html_tag_batch[m].emplace_back("</html>");
            structure_html_tags.push_back(structure_html_tag_batch[m]);
            structure_scores.push_back(structure_score_batch[m]);
            structure_boxes.push_back(structure_boxes_batch[m]);
        }
    }
}

StructureTableRecognizer::StructureTableRecognizer(std::string model_dir, std::string label_path) {
    post_processor_.init(label_path, true);
    LoadModel(model_dir);
}

void StructureTableRecognizer::LoadModel(const std::string& model_dir) {
    paddle_infer::Config config;
    config.SetModel(model_dir + "/inference.pdmodel", model_dir + "/inference.pdiparams");
    config.DisableGpu();
    config.EnableMKLDNN();
    config.SetCpuMathLibraryNumThreads(1);
    config.SwitchUseFeedFetchOps(false);
    config.SwitchSpecifyInputNames(true);
    config.SwitchIrOptim(true);
    config.EnableMemoryOptim();
#ifdef NDEBUG
    config.DisableGlogInfo();
#endif
    predictor_ = paddle_infer::CreatePredictor(config);
}

PaddleStructure::PaddleStructure(bool layout, bool table) {
    if (layout) {
        std::string layout_model_directory;
        std::string layout_dict_path;
        layout_model_ = std::make_unique<StructureLayoutRecognizer>(layout_model_directory, layout_dict_path);
    }
    if (table) {
        std::string table_model_directory;
        std::string table_char_dict_path;
        table_model_ = std::make_unique<StructureTableRecognizer>(table_model_directory, table_char_dict_path);
    }
}

std::vector<StructurePredictResult> PaddleStructure::structure(cv::Mat srcimg, bool layout, bool table, bool ocr) {
    cv::Mat img;
    srcimg.copyTo(img);
    std::vector<StructurePredictResult> structure_results;
    /*if (layout) {
        this->layout(img, structure_results);
    } else {
        StructurePredictResult res;
        res.type = "table";
        res.box = std::vector<float>(4, 0.0);
        res.box[2] = img.cols;
        res.box[3] = img.rows;
        structure_results.push_back(res);
    }
    cv::Mat roi_img;
    for (auto& structure_result : structure_results) {
        roi_img = Utility::crop_image(img, structure_result.box);
        if (structure_result.type == "table" && table) {
            this->table(roi_img, structure_result);
        } else if (ocr) {
            structure_result.text_res = ppocr.ocr(roi_img, true, true, false);
        }
    }*/
    return structure_results;
}

void PaddleStructure::layout(
    cv::Mat img, std::vector<StructurePredictResult>& structure_result) {
    std::vector<double> layout_times;
    layout_model_->Run(img, structure_result);
}

void PaddleStructure::table(cv::Mat img, StructurePredictResult& structure_result) {
    std::vector<std::vector<std::string>> structure_html_tags;
    std::vector<float> structure_scores(1, 0);
    std::vector<std::vector<std::vector<int>>> structure_boxes;
    std::vector<double> structure_times;
    std::vector<cv::Mat> img_list;
    img_list.push_back(img);

    table_model_->Run(img_list, structure_html_tags, structure_scores, structure_boxes);

    std::vector<OCRPredictResult> ocr_result;
    std::string html;
    int expand_pixel = 3;
    for (int i = 0; i < img_list.size(); i++) {
        // det
//        ppocr.det(img_list[i], ocr_result);
        // crop image
        std::vector<cv::Mat> rec_img_list;
        std::vector<int> ocr_box;
        for (int j = 0; j < ocr_result.size(); j++) {
            ocr_box = Utility::xyxyxyxy2xyxy(ocr_result[j].box);
            ocr_box[0] = std::max(0, ocr_box[0] - expand_pixel);
            ocr_box[1] = std::max(0, ocr_box[1] - expand_pixel), ocr_box[2] = std::min(img_list[i].cols, ocr_box[2] + expand_pixel);
            ocr_box[3] = std::min(img_list[i].rows, ocr_box[3] + expand_pixel);
            cv::Mat crop_img = Utility::crop_image(img_list[i], ocr_box);
            rec_img_list.push_back(crop_img);
        }
        // rec
 //       ppocr.rec(rec_img_list, ocr_result);
        // rebuild table
        html = this->rebuild_table(structure_html_tags[i], structure_boxes[i], ocr_result);
        structure_result.html = html;
        structure_result.cell_box = structure_boxes[i];
        structure_result.html_score = structure_scores[i];
    }
}

std::string PaddleStructure::rebuild_table(std::vector<std::string> structure_html_tags, std::vector<std::vector<int>> structure_boxes, std::vector<OCRPredictResult>& ocr_result) {
    // match text in same cell
    std::vector<std::vector<std::string>> matched(structure_boxes.size(), std::vector<std::string>());
    std::vector<int> ocr_box;
    std::vector<int> structure_box;
    for (int i = 0; i < ocr_result.size(); i++) {
        ocr_box = Utility::xyxyxyxy2xyxy(ocr_result[i].box);
        ocr_box[0] -= 1;
        ocr_box[1] -= 1;
        ocr_box[2] += 1;
        ocr_box[3] += 1;
        std::vector<std::vector<float>> dis_list(structure_boxes.size(), std::vector<float>(3, 100000.0));
        for (int j = 0; j < structure_boxes.size(); j++) {
            if (structure_boxes[j].size() == 8) {
                structure_box = Utility::xyxyxyxy2xyxy(structure_boxes[j]);
            } else {
                structure_box = structure_boxes[j];
            }
            dis_list[j][0] = this->dis(ocr_box, structure_box);
            dis_list[j][1] = 1 - Utility::iou(ocr_box, structure_box);
            dis_list[j][2] = j;
        }
        // find min dis idx
        std::ranges::sort(dis_list, [](const std::vector<float>& dis1, const std::vector<float>& dis2) {
            if (dis1[1] < dis2[1]) {
                return true;
            } else if (dis1[1] == dis2[1]) {
                return dis1[0] < dis2[0];
            } else {
                return false;
            }
        });
        matched[dis_list[0][2]].push_back(ocr_result[i].text);
    }

    // get pred html
    std::string html_str;
    int td_tag_idx{};
    for (const auto& structure_html_tag : structure_html_tags) {
        if (structure_html_tag.find("</td>") != std::string::npos) {
            if (structure_html_tag.find("<td></td>") != std::string::npos) {
                html_str += "<td>";
            }
            if (!matched[td_tag_idx].empty()) {
                bool b_with = false;
                if (matched[td_tag_idx][0].find("<b>") != std::string::npos &&
                    matched[td_tag_idx].size() > 1) {
                    b_with = true;
                    html_str += "<b>";
                }
                for (int j = 0; j < matched[td_tag_idx].size(); j++) {
                    std::string content = matched[td_tag_idx][j];
                    if (matched[td_tag_idx].size() > 1) {
                        // remove blank, <b> and </b>
                        if (content.length() > 0 && content.at(0) == ' ') {
                            content = content.substr(0);
                        }
                        if (content.length() > 2 && content.substr(0, 3) == "<b>") {
                            content = content.substr(3);
                        }
                        if (content.length() > 4 &&
                            content.substr(content.length() - 4) == "</b>") {
                            content = content.substr(0, content.length() - 4);
                        }
                        if (content.empty()) {
                            continue;
                        }
                        // add blank
                        if (j != matched[td_tag_idx].size() - 1 &&
                            content.at(content.length() - 1) != ' ') {
                            content += ' ';
                        }
                    }
                    html_str += content;
                }
                if (b_with) {
                    html_str += "</b>";
                }
            }
            if (structure_html_tag.find("<td></td>") != std::string::npos) {
                html_str += "</td>";
            } else {
                html_str += structure_html_tag;
            }
            td_tag_idx += 1;
        } else {
            html_str += structure_html_tag;
        }
    }
    return html_str;
}

float PaddleStructure::dis(std::vector<int>& box1, std::vector<int>& box2) {
    int x1_1 = box1[0];
    int y1_1 = box1[1];
    int x2_1 = box1[2];
    int y2_1 = box1[3];

    int x1_2 = box2[0];
    int y1_2 = box2[1];
    int x2_2 = box2[2];
    int y2_2 = box2[3];

    float dis = std::abs(x1_2 - x1_1) + std::abs(y1_2 - y1_1) + std::abs(x2_2 - x2_1) + std::abs(y2_2 - y2_1);
    float dis_2 = std::abs(x1_2 - x1_1) + std::abs(y1_2 - y1_1);
    float dis_3 = std::abs(x2_2 - x2_1) + std::abs(y2_2 - y2_1);
    return dis + std::min(dis_2, dis_3);
}

}
