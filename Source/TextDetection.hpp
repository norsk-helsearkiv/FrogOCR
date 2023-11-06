#pragma once

#include "Core/Quad.hpp"
#include "Document.hpp"
#include "Settings.hpp"

#include <vector>

namespace frog {

class Image;

class TextDetector {
public:

    TextDetector() = default;
    TextDetector(const TextDetector&) = delete;
    TextDetector(TextDetector&&) = delete;

    virtual ~TextDetector() = default;

    TextDetector& operator=(const TextDetector&) = delete;
    TextDetector& operator=(TextDetector&&) = delete;

    virtual std::vector<Quad> detect(const Image& image, const TextDetectionSettings& settings) = 0;

};

}

#if 0
std::vector<Quad> detect_text_quads(const preprocess::Image& image, const application::Configuration& configuration) {
    auto img = preprocess::pix_to_mat(image.getPix());
    if (img.empty()) {
        log::error("Failed to decode image: {} [{}x{}] {}", image.getPath(), image.getWidth(), image.getHeight(), image.isOk() ? "OK" : "X");
        return {};
    }
    paddle::PPOCR ocr{ true, false, true, true, configuration };
    std::vector<Quad> quads;
    const auto& results = ocr.ocr(img, false, true);
    for (const auto& result : results) {
        if (result.box.size() != 4) {
            log::warning("{} boxes in result.", result.box.size());
            continue;
        }
        Quad quad;
        quad.x1 = static_cast<float>(result.box[0][0]);
        quad.y1 = static_cast<float>(result.box[0][1]);
        quad.x2 = static_cast<float>(result.box[1][0]);
        quad.y2 = static_cast<float>(result.box[1][1]);
        quad.x3 = static_cast<float>(result.box[2][0]);
        quad.y3 = static_cast<float>(result.box[2][1]);
        quad.x4 = static_cast<float>(result.box[3][0]);
        quad.y4 = static_cast<float>(result.box[3][1]);
        quads.push_back(quad);
    }
    return quads;
}
#endif