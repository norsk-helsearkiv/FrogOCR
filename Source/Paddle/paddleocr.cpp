#include "paddleocr.hpp"
#include "Structure.hpp"
#include "Core/String.hpp"
#include "Core/Log.hpp"

namespace frog {

PPOCR::PPOCR(bool cls, bool rec, bool use_angle_cls) {
    /*detector = std::make_unique<Detector>(path_to_string(configuration.detectionModel.value()));
    if (cls && use_angle_cls) {
        std::string cls_model_directory{};
        classifier = std::make_unique<Classifier>(cls_model_directory);
    }
    if (rec) {
        const auto label_path = path_to_string(configuration.recognitionLabels.value());
        recognizer = std::make_unique<Recognizer>(path_to_string(configuration.recognitionModel.value()), label_path);
    }*/
}

std::vector<OCRPredictResult> PPOCR::ocr(cv::Mat image, bool rec, bool cls) const {
    /*const auto boxes = detector->Run(image);

    std::vector<cv::Mat> rotated_cropped_images;
    for (const auto& box : boxes) {
        rotated_cropped_images.push_back(get_rotated_cropped_image(image, box));
    }

    std::vector<OCRPredictResult> ocr_result;

    if (cls) {
        classify(rotated_cropped_images, ocr_result);
    }

    if (rec) {
        recognize(rotated_cropped_images, ocr_result);
    }
    return ocr_result;*/
}

void PPOCR::recognize(std::vector<cv::Mat> img_list, std::vector<OCRPredictResult>& ocr_results) const {
    std::vector<std::string> rec_texts(img_list.size(), "");
    std::vector<float> rec_text_scores(img_list.size(), 0);
    //recognizer->Run(img_list, rec_texts, rec_text_scores);
    for (int i = 0; i < rec_texts.size(); i++) {
        ocr_results[i].text = rec_texts[i];
        ocr_results[i].score = rec_text_scores[i];
    }
}

void PPOCR::classify(std::vector<cv::Mat> img_list, std::vector<OCRPredictResult>& ocr_results) const {
    /*std::vector<int> cls_labels(img_list.size(), 0);
    std::vector<float> cls_scores(img_list.size(), 0);
    classifier->Run(img_list, cls_labels, cls_scores);
    for (int i = 0; i < cls_labels.size(); i++) {
        ocr_results[i].cls_label = cls_labels[i];
        ocr_results[i].cls_score = cls_scores[i];
    }
    double cls_thresh{ 0.9 };
    for (int i = 0; i < img_list.size(); i++) {
        if (ocr_results[i].cls_label % 2 == 1 && ocr_results[i].cls_score > classifier->cls_thresh) {
            cv::rotate(img_list[i], img_list[i], 1);
        }
    }*/
}

void structure(cv::Mat img, bool layout, bool table, bool ocr) {
    PaddleStructure engine{ layout, table };
    std::vector<StructurePredictResult> structure_results = engine.structure(img, layout, table, ocr);
    for (int j = 0; j < structure_results.size(); j++) {
        std::cout << j << "\ttype: " << structure_results[j].type << ", region: [";
        std::cout << structure_results[j].box[0] << ",";
        std::cout << structure_results[j].box[1] << ",";
        std::cout << structure_results[j].box[2] << ",";
        std::cout << structure_results[j].box[3] << "], score: ";
        std::cout << structure_results[j].confidence << ", res: ";
        if (structure_results[j].type == "table") {
            std::cout << structure_results[j].html << std::endl;
        } else if (!structure_results[j].text_res.empty()) {
            Utility::print_result(structure_results[j].text_res);
        }
    }
}

}
