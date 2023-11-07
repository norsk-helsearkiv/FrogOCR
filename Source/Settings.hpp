#pragma once

#include "Core/Log.hpp"

namespace frog {

enum class PageSegmentation { automatic, sparse, block, line, word };

constexpr std::string_view page_segmentation_string(PageSegmentation pageSegmentation) {
    switch (pageSegmentation) {
    case PageSegmentation::automatic: return "Automatic";
    case PageSegmentation::sparse: return "Sparse";
    case PageSegmentation::block: return "Block";
    case PageSegmentation::line: return "Line";
    case PageSegmentation::word: return "Word";
    }
}

struct TextDetectionSettings {
    static constexpr std::string_view defaultTextDetector{ "Paddle" };

    std::string textDetector{ defaultTextDetector };

    // [0.0f - 1.0f]
    std::optional<float> cropX;
    std::optional<float> cropY;
    std::optional<float> cropWidth;
    std::optional<float> cropHeight;

    std::unordered_map<std::string, std::string> other;

    [[nodiscard]] std::optional<std::string> find(const std::string& name) const {
        if (const auto it = other.find(name); it != other.end()) {
            return it->second;
        } else {
            return std::nullopt;
        }
    }
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
    std::optional<std::string> textAngleClassifier;
    bool overwriteOutput{};

    Settings() = default;

    Settings(std::string_view settings) {
        for (const auto setting: split_string_view(settings, ",")) {
            if (const auto pair = split_string_view(setting, "="); pair.size() == 2) {
                set(pair[0], pair[1]);
            } else {
                log::warning("Invalid setting format: {}", setting);
            }
        }
    }

    [[nodiscard]] std::string csv() const {
        std::string result;

        // Detection
        result += fmt::format("TextDetector={},", detection.textDetector);
        for (const auto& [key, value] : detection.other) {
            result += fmt::format("TextDetection.{}={},", key, value);
        }

        // Angle Classification
        if (textAngleClassifier.has_value()) {
            result += fmt::format("TextAngleClassifier={},", textAngleClassifier.value());
        }

        // Recognition
        result += fmt::format("SauvolaKFactor={},", recognition.sauvolaKFactor);
        result += fmt::format("PageSegmentation={},", page_segmentation_string(recognition.pageSegmentation));
        if (!recognition.characterWhitelist.empty()) {
            result += fmt::format("CharacterWhitelist={},", recognition.characterWhitelist);
        }
        result += fmt::format("TextRecognizer={},", recognition.textRecognizer);

        // Misc
        result += fmt::format("OverwriteOutput={},", overwriteOutput ? "true" : "false");

        if (result.ends_with(",")) {
            result.pop_back();
        }
        return result;
    }

    void set(std::string_view key, std::string_view value) {
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
        } else if (key == "TextAngleClassifier") {
            if (!value.empty()) {
                textAngleClassifier = value;
            }
        } else if (key == "OverwriteOutput") {
            overwriteOutput = value == "true";
        } else {
            if (key.starts_with("TextDetection.")) {
                const std::string relativeKey{ key.substr(std::string_view{"TextDetection."}.size()) };
                detection.other[relativeKey] = value;
            }
        }
    }
};

}
