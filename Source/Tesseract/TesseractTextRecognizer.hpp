#pragma once

#include "TextRecognizer.hpp"
#include "Config.hpp"

#include <tesseract/baseapi.h>
#include <tesseract/ocrclass.h>

namespace frog {

class TesseractTextRecognizer : public TextRecognizer {
public:

    TesseractTextRecognizer(const TesseractConfig& config);

    Document recognize(PIX* image, const std::vector<Quad>& quads, std::vector<int> angles, const TextRecognitionSettings& settings) const override;

private:

    mutable tesseract::TessBaseAPI tesseract;

};

}
