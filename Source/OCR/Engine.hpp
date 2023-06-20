#pragma once

#include "Confidence.hpp"
#include "PageSegmentation.hpp"

#include <memory>
#include <filesystem>
#include <functional>
#include <optional>

namespace tesseract {
class ResultIterator;
}

namespace frog::preprocess {
class Image;
}

namespace frog::ocr {

class Dataset;

class Engine {
public:

    Engine(const Dataset& dataset);
    Engine(const Engine&) = delete;
    Engine(Engine&&) = delete;

    ~Engine();

    Engine& operator=(const Engine&) = delete;
    Engine& operator=(Engine&&) = delete;

    bool isOk() const;

    void setSauvolaKFactor(float sauvolaKFactor);
    void setPageSegmentationMode(PageSegmentation mode);
    void setImage(const preprocess::Image& image);
    void setRectangle(int x, int y, int width, int height);
    void setCharacterWhitelist(std::string_view characterWhitelist);

    void setProgressCallback(std::function<void(int progress, int left, int right, int top, int bottom)> callback, int minDeltaForCallback);
    void setCancelCallback(std::function<bool(void* cancelThis, int wordCount)> callback);
    void recognizeAll();

    Confidence getConfidence() const;

    tesseract::ResultIterator* getResultIterator();

private:

    class InternalEngine;

    std::unique_ptr<InternalEngine> engine;
    std::optional<float> currentSauvolaKFactor;
    std::optional<PageSegmentation> pageSegmentationMode;
    std::string characterWhitelist;

};

}
