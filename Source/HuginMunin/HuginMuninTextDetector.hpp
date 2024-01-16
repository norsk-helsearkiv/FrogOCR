#pragma once

#include "TextDetection.hpp"
#include "Image.hpp"
#include "Config.hpp"

namespace frog {

class HuginMuninTextDetector : public TextDetector {
public:

    HuginMuninTextDetector(const HuginMuninTextDetectorConfig& config);

    std::vector<Quad> detect(PIX* image, const TextDetectionSettings& settings) const override;

private:

    const HuginMuninTextDetectorConfig& config;
    int instanceId{};
    std::string instanceTemporaryStorageDirectory;
    std::string pythonScriptPath;

};

}
