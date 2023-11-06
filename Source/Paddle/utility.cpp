#include "utility.hpp"
#include <iostream>
#include <ostream>

#include <vector>

#include <sys/stat.h>

namespace frog {

std::vector<std::string> Utility::ReadDict(const std::string& path) {
    std::ifstream in(path);
    std::string line;
    std::vector<std::string> m_vec;
    if (in) {
        while (getline(in, line)) {
            m_vec.push_back(line);
        }
    } else {
        std::cout << "no such label file: " << path << ", exit the program..." << std::endl;
        exit(1);
    }
    return m_vec;
}

void Utility::print_result(const std::vector<OCRPredictResult>& ocr_result) {
    for (int i = 0; i < ocr_result.size(); i++) {
        std::cout << i << "\t";
        // det
        std::vector<std::vector<int>> boxes = ocr_result[i].box;
        if (!boxes.empty()) {
            std::cout << "det boxes: [";
            for (int n = 0; n < boxes.size(); n++) {
                std::cout << '[' << boxes[n][0] << ',' << boxes[n][1] << "]";
                if (n != boxes.size() - 1) {
                    std::cout << ',';
                }
            }
            std::cout << "] ";
        }
        // rec
        if (ocr_result[i].score != -1.0) {
            std::cout << "rec text: " << ocr_result[i].text << " rec score: " << ocr_result[i].score << " ";
        }
        // cls
        if (ocr_result[i].cls_label != -1) {
            std::cout << "cls label: " << ocr_result[i].cls_label << " cls score: " << ocr_result[i].cls_score;
        }
        std::cout << std::endl;
    }
}

cv::Mat Utility::crop_image(cv::Mat& img, const std::vector<int>& box) {
    cv::Mat crop_im;
    int crop_x1 = std::max(0, box[0]);
    int crop_y1 = std::max(0, box[1]);
    int crop_x2 = std::min(img.cols - 1, box[2] - 1);
    int crop_y2 = std::min(img.rows - 1, box[3] - 1);

    crop_im = cv::Mat::zeros(box[3] - box[1], box[2] - box[0], 16);
    cv::Mat crop_im_window = crop_im(cv::Range(crop_y1 - box[1], crop_y2 + 1 - box[1]), cv::Range(crop_x1 - box[0], crop_x2 + 1 - box[0]));
    cv::Mat roi_img = img(cv::Range(crop_y1, crop_y2 + 1), cv::Range(crop_x1, crop_x2 + 1));
    crop_im_window += roi_img;
    return crop_im;
}

cv::Mat Utility::crop_image(cv::Mat& img, const std::vector<float>& box) {
    std::vector<int> box_int{ (int) box[0], (int) box[1], (int) box[2], (int) box[3] };
    return crop_image(img, box_int);
}

std::vector<int> Utility::xyxyxyxy2xyxy(std::vector<std::vector<int>>& box) {
    int x_collect[4] = { box[0][0], box[1][0], box[2][0], box[3][0] };
    int y_collect[4] = { box[0][1], box[1][1], box[2][1], box[3][1] };
    int left = int(*std::min_element(x_collect, x_collect + 4));
    int right = int(*std::max_element(x_collect, x_collect + 4));
    int top = int(*std::min_element(y_collect, y_collect + 4));
    int bottom = int(*std::max_element(y_collect, y_collect + 4));
    std::vector<int> box1(4, 0);
    box1[0] = left;
    box1[1] = top;
    box1[2] = right;
    box1[3] = bottom;
    return box1;
}

std::vector<int> Utility::xyxyxyxy2xyxy(std::vector<int>& box) {
    int x_collect[4] = { box[0], box[2], box[4], box[6] };
    int y_collect[4] = { box[1], box[3], box[5], box[7] };
    int left = int(*std::min_element(x_collect, x_collect + 4));
    int right = int(*std::max_element(x_collect, x_collect + 4));
    int top = int(*std::min_element(y_collect, y_collect + 4));
    int bottom = int(*std::max_element(y_collect, y_collect + 4));
    std::vector<int> box1(4, 0);
    box1[0] = left;
    box1[1] = top;
    box1[2] = right;
    box1[3] = bottom;
    return box1;
}

float Utility::fast_exp(float x) {
    union {
        std::uint32_t i;
        float f;
    } v{};
    v.i = (1 << 23) * (1.4426950409f * x + 126.93490512f);
    return v.f;
}

std::vector<float> Utility::activation_function_softmax(std::vector<float>& src) {
    int length = src.size();
    std::vector<float> dst;
    dst.resize(length);
    const auto alpha = static_cast<float>(*std::max_element(&src[0], &src[0 + length]));
    float denominator{};
    for (int i = 0; i < length; ++i) {
        dst[i] = fast_exp(src[i] - alpha);
        denominator += dst[i];
    }
    for (int i = 0; i < length; ++i) {
        dst[i] /= denominator;
    }
    return dst;
}

float Utility::iou(std::vector<int>& box1, std::vector<int>& box2) {
    int area1 = std::max(0, box1[2] - box1[0]) * std::max(0, box1[3] - box1[1]);
    int area2 = std::max(0, box2[2] - box2[0]) * std::max(0, box2[3] - box2[1]);

    // computing the sum_area
    int sum_area = area1 + area2;

    // find the each point of intersect rectangle
    int x1 = std::max(box1[0], box2[0]);
    int y1 = std::max(box1[1], box2[1]);
    int x2 = std::min(box1[2], box2[2]);
    int y2 = std::min(box1[3], box2[3]);

    // judge if there is an intersection
    if (y1 >= y2 || x1 >= x2) {
        return 0.0;
    } else {
        int intersect = (x2 - x1) * (y2 - y1);
        return intersect / (sum_area - intersect + 0.00000001f);
    }
}

float Utility::iou(std::vector<float>& box1, std::vector<float>& box2) {
    float area1 = std::max((float) 0.0, box1[2] - box1[0]) * std::max((float) 0.0, box1[3] - box1[1]);
    float area2 = std::max((float) 0.0, box2[2] - box2[0]) * std::max((float) 0.0, box2[3] - box2[1]);

    // computing the sum_area
    float sum_area = area1 + area2;

    // find the each point of intersect rectangle
    float x1 = std::max(box1[0], box2[0]);
    float y1 = std::max(box1[1], box2[1]);
    float x2 = std::min(box1[2], box2[2]);
    float y2 = std::min(box1[3], box2[3]);

    // judge if there is an intersection
    if (y1 >= y2 || x1 >= x2) {
        return 0.0f;
    } else {
        float intersect = (x2 - x1) * (y2 - y1);
        return intersect / (sum_area - intersect + 0.00000001f);
    }
}

}
