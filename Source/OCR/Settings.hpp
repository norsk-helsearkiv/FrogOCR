#pragma once

#include "Core/Log.hpp"
#include "PageSegmentation.hpp"

namespace frog::ocr {

struct Settings {
    static constexpr float defaultSauvolaKFactor{ 0.34f };

    float sauvolaKFactor{ defaultSauvolaKFactor };
    PageSegmentation pageSegmentation{ PageSegmentation::block };
    std::optional<int> cropX;
    std::optional<int> cropY;
    std::optional<int> cropWidth;
    std::optional<int> cropHeight;
    std::optional<float> minWordConfidence;
    std::string characterWhitelist;

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
                sauvolaKFactor = from_string<float>(value).value_or(defaultSauvolaKFactor);
            } else if (key == "PageSegmentation") {
                if (value == "Automatic") {
                    pageSegmentation = PageSegmentation::automatic;
                } else if (value == "Block") {
                    pageSegmentation = PageSegmentation::block;
                } else if (value == "Line") {
                    pageSegmentation = PageSegmentation::line;
                } else if (value == "Word") {
                    pageSegmentation = PageSegmentation::word;
                } else if (value == "Sparse") {
                    pageSegmentation = PageSegmentation::sparse;
                } else {
                    log::warning("Failed to read setting: {} = {}", key, value);
                }
            } else if (key == "CropX") {
                cropX = from_string<int>(value);
                if (!cropX.has_value()) {
                    log::warning("Failed to read setting: {} = {}", key, value);
                }
            } else if (key == "CropY") {
                cropY = from_string<int>(value);
                if (!cropY.has_value()) {
                    log::warning("Failed to read setting: {} = {}", key, value);
                }
            } else if (key == "CropWidth") {
                cropWidth = from_string<int>(value);
                if (!cropWidth.has_value()) {
                    log::warning("Failed to read setting: {} = {}", key, value);
                }
            } else if (key == "CropHeight") {
                cropHeight = from_string<int>(value);
                if (!cropHeight.has_value()) {
                    log::warning("Failed to read setting: {} = {}", key, value);
                }
            } else if (key == "MinWordConfidence") {
                minWordConfidence = from_string<float>(value);
                if (!minWordConfidence.has_value()) {
                    log::warning("Failed to read setting: {} = {}", key, value);
                }
            } else if (key == "CharacterWhitelist") {
                characterWhitelist = value;
            } else {
                log::warning("Unknown setting: {} = {}", key, value);
            }
        }
    }
};

}
