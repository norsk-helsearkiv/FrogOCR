#pragma once

#include "TextRecognizer.hpp"
#include "Config.hpp"

#include <regex>

namespace frog {

class HuginMuninTextRecognizer : public TextRecognizer {
public:

    struct InputData {
        std::vector<float> quadScaleFactors;
        std::string imagePathList;
    };

    HuginMuninTextRecognizer(const HuginMuninTextRecognizerConfig& config);

    Document recognize(PIX* image, const std::vector<Quad>& quads, std::vector<int> angles, const TextRecognitionSettings& settings) const override;

private:

    [[nodiscard]] InputData createInputData(PIX* image, const std::vector<Quad>& quads) const;
    [[nodiscard]] std::string executeProcessAndReturnResult(const std::string& command) const;
    [[nodiscard]] std::pair<std::optional<std::size_t>, std::string_view> parsePyLaiaLineForIndexAndOutput(std::string_view line) const;

    std::vector<Word> parseSegmentationWords(std::string_view segmentationLine) const;

    const HuginMuninTextRecognizerConfig& config;
    int instanceId{};
    std::string instanceTemporaryStorageDirectory;

};

}
