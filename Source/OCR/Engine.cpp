#include "Engine.hpp"
#include "Dataset.hpp"
#include "PreProcess/Image.hpp"
#include "Core/String.hpp"
#include "Core/Log.hpp"

#include <tesseract/baseapi.h>
#include <tesseract/ocrclass.h>

namespace frog::ocr {

static thread_local int progressChangeRequiredToNotifyProgressCallback{ 1 };
static thread_local std::function<void(int, int, int, int, int)> threadLocalTesseractProgressCallback;
static thread_local std::function<bool(void*, int)> threadLocalTesseractCancelCallback;

static bool globalTesseractProgressCallback(int progress, int left, int right, int top, int bottom) {
	thread_local int lastProgress{};
	if (std::abs(lastProgress - progress) >= progressChangeRequiredToNotifyProgressCallback) {
		lastProgress = progress;
		threadLocalTesseractProgressCallback(progress, left, right, top, bottom);
	}
	return true; // return value means nothing.
}

static bool globalTesseractCancelCallback(void* cancelThis, int wordCount) {
	return threadLocalTesseractCancelCallback(cancelThis, wordCount);
}

class Engine::InternalEngine {
public:

	static constexpr std::string_view sauvolaThresholdingMethod{ "2" };

	tesseract::TessBaseAPI tesseract;
	bool ok{ false };

	InternalEngine(const Dataset& dataset) {
		if (tesseract.Init(path_to_string(dataset.getPath()).c_str(), dataset.getName().data(), tesseract::OcrEngineMode::OEM_LSTM_ONLY)) {
			log::error("Failed to initialize Tesseract.");
			return;
		}
		ok = setVariable("thresholding_method", sauvolaThresholdingMethod);
		setVariable("lstm_choice_mode", "2");
		setVariable("tessedit_write_images", "true");
	}

	bool setVariable(std::string_view name, std::string_view value) {
        log::info("Changing {} from {} to {}", name, getVariable(name), value);
		if (tesseract.SetVariable(name.data(), value.data())) {
			return true;
		}
        log::warning("Failed to set {} to {}.", name, value);
		return false;
	}

	std::string getVariable(std::string_view name) const {
		std::string value;
		tesseract.GetVariableAsString(name.data(), &value);
		return value;
	}

	void recognizeAll() {
		tesseract::ETEXT_DESC monitor;
		monitor.progress_callback = threadLocalTesseractProgressCallback ? globalTesseractProgressCallback : nullptr;
		monitor.cancel = threadLocalTesseractCancelCallback ? globalTesseractCancelCallback : nullptr;
		if (tesseract.Recognize(&monitor) != 0) {
            log::error("Failed to recognize image with tesseract.");
		}
		threadLocalTesseractProgressCallback = {};
		threadLocalTesseractCancelCallback = {};
	}

};

Engine::Engine(const Dataset& dataset) {
	engine = std::make_unique<InternalEngine>(dataset);
}

Engine::~Engine() {
	// for internal engine destructor.
}

bool Engine::isOk() const {
	return engine->ok;
}

void Engine::setSauvolaKFactor(float sauvolaKFactor) {
	if (currentSauvolaKFactor != sauvolaKFactor) {
		currentSauvolaKFactor = sauvolaKFactor;
		engine->setVariable("thresholding_kfactor", std::to_string(sauvolaKFactor));
	}
}

void Engine::setPageSegmentationMode(PageSegmentation mode) {
	if (pageSegmentationMode != mode) {
		pageSegmentationMode = mode;
		if (mode == PageSegmentation::automatic) {
            engine->tesseract.SetPageSegMode(tesseract::PageSegMode::PSM_AUTO);
        } else if (mode == PageSegmentation::block) {
            engine->tesseract.SetPageSegMode(tesseract::PageSegMode::PSM_SINGLE_BLOCK);
        } else if (mode == PageSegmentation::line) {
			engine->tesseract.SetPageSegMode(tesseract::PageSegMode::PSM_SINGLE_LINE);
		} else if (mode == PageSegmentation::word) {
			engine->tesseract.SetPageSegMode(tesseract::PageSegMode::PSM_SINGLE_WORD);
		} else if (mode == PageSegmentation::sparse) {
            engine->tesseract.SetPageSegMode(tesseract::PageSegMode::PSM_SPARSE_TEXT);
        }
    }
}

void Engine::setImage(const preprocess::Image& image) {
	engine->tesseract.SetImage(image.getPix());
}

void Engine::setRectangle(int x, int y, int width, int height) {
	engine->tesseract.SetRectangle(x, y, width, height);
}

void Engine::setCharacterWhitelist(std::string_view whitelist) {
    if (characterWhitelist != whitelist) {
        characterWhitelist = whitelist;
        engine->setVariable("tessedit_char_whitelist", characterWhitelist);
    }
}

void Engine::setProgressCallback(std::function<void(int, int, int, int, int)> callback, int minDeltaForCallback) {
	threadLocalTesseractProgressCallback = callback;
	progressChangeRequiredToNotifyProgressCallback = minDeltaForCallback;
}

void Engine::setCancelCallback(std::function<bool(void*, int)> callback) {
	threadLocalTesseractCancelCallback = callback;
}

void Engine::recognizeAll() {
	engine->recognizeAll();
}

Confidence Engine::getConfidence() const {
	return { static_cast<float>(engine->tesseract.MeanTextConf()), Confidence::Format::percent };
}

tesseract::ResultIterator* Engine::getResultIterator() {
	return engine->tesseract.GetIterator();
}

}
