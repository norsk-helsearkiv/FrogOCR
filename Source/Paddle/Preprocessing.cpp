#include "Preprocessing.hpp"
#include "opencv2/imgproc.hpp"

namespace frog {

void normalize(cv::Mat* im, const std::vector<float>& mean, const std::vector<float>& scale, bool is_scale) {
    im->convertTo(*im, CV_32FC3, is_scale ? 1.0 / 255.0 : 1.0);
    std::vector<cv::Mat> bgr_channels(3);
    cv::split(*im, bgr_channels);
    for (auto i = 0; i < bgr_channels.size(); i++) {
        bgr_channels[i].convertTo(bgr_channels[i], CV_32FC1, 1.0f * scale[i], (0.0f - mean[i]) * scale[i]);
    }
    cv::merge(bgr_channels, *im);
}

void permute_rgb_to_chw(const cv::Mat& im, float* data) {
    for (int channel{ 0 }; channel < im.channels(); channel++) {
        cv::extractChannel(im, cv::Mat(im.rows, im.cols, CV_32FC1, data + channel * im.rows * im.cols), channel);
    }
}

void permute_rgb_to_chw_batch(const std::vector<cv::Mat>& images, float* data) {
    for (int j = 0; j < images.size(); j++) {
        int rh = images[j].rows;
        int rw = images[j].cols;
        int rc = images[j].channels();
        for (int i = 0; i < rc; ++i) {
            cv::extractChannel(images[j], cv::Mat(rh, rw, CV_32FC1, data + (j * rc + i) * rh * rw), i);
        }
    }
}

void resize_image(const cv::Mat& img, cv::Mat& resize_img, int limit_side_len, float& ratio_h, float& ratio_w) {
    int w = img.cols;
    int h = img.rows;
    float ratio = 1.0f;
    int max_wh = std::max(h, w);
    if (max_wh > limit_side_len) {
        if (h > w) {
            ratio = static_cast<float>(limit_side_len) / static_cast<float>(h);
        } else {
            ratio = static_cast<float>(limit_side_len) / static_cast<float>(w);
        }
    }
    const auto resize_h = std::max(static_cast<int>(std::round(static_cast<float>(h) * ratio / 32.0f) * 32.0f), 32);
    const auto resize_w = std::max(static_cast<int>(std::round(static_cast<float>(w) * ratio / 32.0f) * 32.0f), 32);
    cv::resize(img, resize_img, cv::Size(resize_w, resize_h));
    ratio_h = static_cast<float>(resize_h) / static_cast<float>(h);
    ratio_w = static_cast<float>(resize_w) / static_cast<float>(w);
}

void recognition_resize_image(const cv::Mat& img, cv::Mat& resize_img, float wh_ratio, const std::vector<int>& rec_image_shape) {
    int imgH = rec_image_shape[1];
    int imgW = int(imgH * wh_ratio);
    float ratio = float(img.cols) / float(img.rows);
    int resize_w{};
    if (ceilf(imgH * ratio) > imgW) {
        resize_w = imgW;
    } else {
        resize_w = int(ceilf(imgH * ratio));
    }
    cv::resize(img, resize_img, cv::Size(resize_w, imgH), 0.f, 0.f, cv::INTER_LINEAR);
    cv::copyMakeBorder(resize_img, resize_img, 0, 0, 0, int(imgW - resize_img.cols), cv::BORDER_CONSTANT, { 0, 0, 0 });
}

void table_resize_image(const cv::Mat& img, cv::Mat& resize_img, int max_len) {
    int w = img.cols;
    int h = img.rows;
    float ratio = w >= h ? float(max_len) / float(w) : float(max_len) / float(h);
    int resize_h = int(float(h) * ratio);
    int resize_w = int(float(w) * ratio);
    cv::resize(img, resize_img, cv::Size(resize_w, resize_h));
}

void table_pad_image(const cv::Mat& img, cv::Mat& resize_img, int max_len) {
    int w = img.cols;
    int h = img.rows;
    cv::copyMakeBorder(img, resize_img, 0, max_len - h, 0, max_len - w, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
}

void resize(const cv::Mat& img, cv::Mat& resize_img, int h, int w) {
    cv::resize(img, resize_img, cv::Size(w, h));
}

}
