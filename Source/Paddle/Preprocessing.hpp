#pragma once

#include <vector>

#include "opencv2/core.hpp"

namespace frog {

void normalize(cv::Mat* im, const std::vector<float>& mean, const std::vector<float>& scale, bool is_scale);
void permute_rgb_to_chw(const cv::Mat& im, float* data);
void permute_rgb_to_chw_batch(const std::vector<cv::Mat>& images, float* data);
void resize_image(const cv::Mat& img, cv::Mat& resize_img, int limit_side_len, float& ratio_h, float& ratio_w);
void recognition_resize_image(const cv::Mat& img, cv::Mat& resize_img, float wh_ratio, const std::vector<int>& rec_image_shape);
void classification_resize_image(const cv::Mat& img, cv::Mat& resize_img, const std::vector<int>& rec_image_shape);
void table_resize_image(const cv::Mat& img, cv::Mat& resize_img, int max_len);
void table_pad_image(const cv::Mat& img, cv::Mat& resize_img, int max_len);
void resize(const cv::Mat& img, cv::Mat& resize_img, int h, int w);

}
