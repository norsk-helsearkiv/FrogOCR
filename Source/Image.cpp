#include "Image.hpp"
#include "Core/Log.hpp"
#include "Core/Quad.hpp"

namespace frog {

PIX* copy_pixels_in_quad(PIX* source, const Quad& quad) {
    const auto top = static_cast<int>(quad.top());
    const auto bottom = static_cast<int>(quad.bottom());
    const auto left = static_cast<int>(quad.left());
    const auto right = static_cast<int>(quad.right());
    auto destination = pixCreate(right - left, bottom - top, static_cast<int>(source->d));
    pixSetBlackOrWhite(destination, L_SET_WHITE);
    for (int y{ top }; y <= bottom; y++) {
        for (int x{ left }; x <= right; x++) {
            if (quad.contains(static_cast<float>(x), static_cast<float>(y))) {
                l_uint32 pixel{};
                pixGetPixel(source, x, y, &pixel);
                pixSetPixel(destination, x - left, y - top, pixel);
            }
        }
    }
    return destination;
}

cv::Mat pix_to_mat(PIX* pix) {
    if (!pix) {
        log::error("Pix is nullptr.");
        return {};
    }
    const int width = pixGetWidth(pix);
    const int height = pixGetHeight(pix);
    const int depth = pixGetDepth(pix);
    cv::Mat mat{ cv::Size(width, height), depth <= 8 ? CV_8UC1 : CV_8UC3 };
    if (depth <= 8) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                l_uint32 val{};
                pixGetPixel(pix, x, y, &val);
                mat.at<uchar>(cv::Point{ x, y }) = static_cast<uchar>(255 * val);
            }
        }
    } else if (depth >= 32) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                l_int32 r{};
                l_int32 g{};
                l_int32 b{};
                pixGetRGBPixel(pix, x, y, &r, &g, &b);
                mat.at<cv::Vec3b>(cv::Point{ x, y }) = { static_cast<std::uint8_t>(b), static_cast<std::uint8_t>(g), static_cast<std::uint8_t>(r) };
            }
        }
    } else {
        log::error("Unsupported bit depth: {}", depth);
    }
    return mat;
}

}
