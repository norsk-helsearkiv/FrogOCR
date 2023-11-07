#pragma once

#include "TextRecognizer.hpp"
#include "Config.hpp"

#include <tesseract/baseapi.h>
#include <tesseract/ocrclass.h>

namespace frog {

class TesseractTextRecognizer : public TextRecognizer {
public:

    TesseractTextRecognizer(const TesseractConfig& config);

    Document recognize(const Image& image, const std::vector<Quad>& quads, std::vector<int> angles, const TextRecognitionSettings& settings) override;

private:

    tesseract::TessBaseAPI tesseract;

};

}
