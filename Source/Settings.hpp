#pragma once

#include "Core/Log.hpp"

namespace frog {

enum class PageSegmentation { automatic, sparse, block, word, line };

struct TextDetectionSettings {
    static constexpr std::string_view defaultTextDetector{ "Tesseract" };

    std::string textDetector{ defaultTextDetector };

    // [0.0f - 1.0f]
    std::optional<float> cropX;
    std::optional<float> cropY;
    std::optional<float> cropWidth;
    std::optional<float> cropHeight;
};

struct TextRecognitionSettings {
    static constexpr float defaultSauvolaKFactor{ 0.34f };
    static constexpr std::string_view defaultTextRecognizer{ "Tesseract" };

    std::string textRecognizer{ defaultTextRecognizer };
    float sauvolaKFactor{ defaultSauvolaKFactor };
    PageSegmentation pageSegmentation{ PageSegmentation::block };
    std::string characterWhitelist;
    std::optional<float> minWordConfidence;
};

struct Settings {

    TextDetectionSettings detection;
    TextRecognitionSettings recognition;
    std::optional<std::string> orientationClassifier;
    bool overwriteOutput{};

    Settings() = default;

    Settings(std::string_view settings) {
        for (const auto setting: split_string_view(settings, ",")) {
            const auto keyValue = split_string_view(setting, "=");
            if (keyValue.size() != 2) {
                log::warning("Invalid setting format: {}", setting);
                continue;
            }
            const auto key = keyValue[0];
            const auto value = keyValue[1];
            if (key == "SauvolaKFactor") {
                recognition.sauvolaKFactor = from_string<float>(value).value_or(TextRecognitionSettings::defaultSauvolaKFactor);
            } else if (key == "PageSegmentation") {
                if (value == "Automatic") {
                    recognition.pageSegmentation = PageSegmentation::automatic;
                } else if (value == "Block") {
                    recognition.pageSegmentation = PageSegmentation::block;
                } else if (value == "Line") {
                    recognition.pageSegmentation = PageSegmentation::line;
                } else if (value == "Word") {
                    recognition.pageSegmentation = PageSegmentation::word;
                } else if (value == "Sparse") {
                    recognition.pageSegmentation = PageSegmentation::sparse;
                } else {
                    log::warning("Failed to read setting: {} = {}", key, value);
                }
            } else if (key == "CropX") {
                detection.cropX = from_string<int>(value);
                if (!detection.cropX.has_value()) {
                    log::warning("Failed to read setting: {} = {}", key, value);
                }
            } else if (key == "CropY") {
                detection.cropY = from_string<int>(value);
                if (!detection.cropY.has_value()) {
                    log::warning("Failed to read setting: {} = {}", key, value);
                }
            } else if (key == "CropWidth") {
                detection.cropWidth = from_string<int>(value);
                if (!detection.cropWidth.has_value()) {
                    log::warning("Failed to read setting: {} = {}", key, value);
                }
            } else if (key == "CropHeight") {
                detection.cropHeight = from_string<int>(value);
                if (!detection.cropHeight.has_value()) {
                    log::warning("Failed to read setting: {} = {}", key, value);
                }
            } else if (key == "MinWordConfidence") {
                recognition.minWordConfidence = from_string<float>(value);
                if (!recognition.minWordConfidence.has_value()) {
                    log::warning("Failed to read setting: {} = {}", key, value);
                }
            } else if (key == "CharacterWhitelist") {
                recognition.characterWhitelist = value;
            } else if (key == "TextDetector") {
                detection.textDetector = value;
            } else if (key == "TextRecognizer") {
                recognition.textRecognizer = value;
            } else if (key == "OrientationClassifier") {
                if (!value.empty()) {
                    orientationClassifier = value;
                }
            } else if (key == "OverwriteOutput") {
                overwriteOutput = value == "true";
            } else {
                log::warning("Unknown setting: {} = {}", key, value);
            }
        }
    }
};

}
