#include "IntegratedTextDetector.hpp"
#include "Image.hpp"

namespace frog {

std::vector<Quad> IntegratedTextDetector::detect(const Image& image, const TextDetectionSettings& settings) {
    float x{};
    float y{};
    auto width = static_cast<float>(image.getWidth());
    auto height = static_cast<float>(image.getHeight());
    if (settings.cropX.has_value()) {
        x *= settings.cropX.value();
    }
    if (settings.cropY.has_value()) {
        y *= settings.cropY.value();
    }
    if (settings.cropWidth.has_value()) {
        width *= settings.cropWidth.value();
    }
    if (settings.cropHeight.has_value()) {
        height *= settings.cropHeight.value();
    }
    Quad quad;
    quad.x1 = x;
    quad.y1 = y;
    quad.x2 = x + width;
    quad.y2 = y;
    quad.x3 = x + width;
    quad.y3 = y + height;
    quad.x4 = x;
    quad.y4 = y + height;
    return { quad };
}

}
