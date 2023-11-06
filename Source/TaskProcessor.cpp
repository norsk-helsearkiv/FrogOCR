#include "TaskProcessor.hpp"

namespace frog {

TaskProcessor::TaskProcessor(const Profile& profile) {
    integratedTextDetector = std::make_unique<IntegratedTextDetector>();
    if (profile.paddleTextDetector.has_value()) {
        paddleTextDetector = std::make_unique<PaddleTextDetector>(profile.paddleTextDetector.value());
    }
    if (profile.tesseract.has_value()) {
        tesseractTextRecognizer = std::make_unique<TesseractTextRecognizer>(profile.tesseract.value());
    }
    if (profile.paddleTextOrientationClassifier.has_value()) {
        paddleTextOrientationClassifier = std::make_unique<PaddleTextOrientationClassifier>(profile.paddleTextOrientationClassifier.value());
    }
}

TaskProcessor::~TaskProcessor() {
    if (thread.joinable()) {
        thread.join();
    }
}

void TaskProcessor::pushTask(Task task) {
    std::lock_guard lock{ taskMutex };
    tasks.emplace_back(task);
    remainingTaskCount++;
}

bool TaskProcessor::isFinished() const {
    return finished;
}

void TaskProcessor::relaunch() {
    if (!finished) {
        log::error("Attempted to relaunch thread before joining with current one.");
        return;
    }
    if (thread.joinable()) {
        thread.join();
    }
    finished = false;
    thread = std::thread{ [this] {
        while (!finished) {
            {
                std::lock_guard lock{ taskMutex };
                activeTask = tasks.front();
                tasks.erase(tasks.begin());
                remainingTaskCount--;
            }
            doTask(activeTask);
            if (tasks.empty()) {
                finished = true;
            }
        }
    }};
}

int TaskProcessor::getRemainingTaskCount() const {
    return remainingTaskCount;
}

void TaskProcessor::doTask(const Task& task) {
    const Settings settings{ task.settingsCsv };

    // Pre-checks
    if (!settings.overwriteOutput && std::filesystem::exists(task.outputPath)) {
        log::warning("Output file already exists: %cyan{}", task.outputPath);
        return;
    }
    if (!std::filesystem::exists(task.inputPath)) {
        log::error("Input file does not exist: %cyan{}", task.inputPath);
        return;
    }

    // Initialize
    Image image{ task.inputPath };
    if (!image.isOk()) {
        log::error("Failed to load image: %cyan{}", task.inputPath);
        return;
    }

    TextDetector* textDetector{};
    if (settings.detection.textDetector == "Paddle") {
        textDetector = paddleTextDetector.get();
    } else {
        textDetector = integratedTextDetector.get();
    }

    TextRecognizer* textRecognizer{};
    if (settings.recognition.textRecognizer == "Tesseract") {
        textRecognizer = tesseractTextRecognizer.get();
    } else if (settings.recognition.textRecognizer == "Paddle") {
        log::error("Paddle text recognition not yet supported.");
        return;
    } else {
        log::error("Unknown text recognizer: {}", settings.recognition.textRecognizer);
        return;
    }

    // Text Detection
    const auto& quads = textDetector->detect(image, settings.detection);

    // Text Orientation Classification
    std::vector<int> angles;
    angles.resize(quads.size(), 0);

    if (paddleTextOrientationClassifier && settings.orientationClassifier == "Paddle") {
        const auto& classifications = paddleTextOrientationClassifier->classify(image, quads);
        std::size_t angledCount{};
        for (std::size_t i{}; i < quads.size(); i++) {
            if (classifications[i].angle() == 180 && classifications[i].confidence > 0.95f) {
                angles[i] = 180;
                angledCount++;
            }
        }
        // If most quads have high confidence of being 180 degrees flipped, lower the confidence required.
        if (angledCount > quads.size() / 2) {
            for (std::size_t i{}; i < quads.size(); i++) {
                if (classifications[i].angle() == 180 && classifications[i].confidence > 0.7f) {
                    angles[i] = 180;
                }
            }
        }
    }

    // Text Recognition
    auto ocrDocument = textRecognizer->recognize(image, quads, angles, settings.recognition);

    // Create Alto
    alto::Alto alto{ ocrDocument, image, settings };
    if (!write_file(task.outputPath, alto::to_xml(alto))) {
        log::error("Failed to write AltoXML file: {}", task.outputPath);
    }
}

}

#ifndef NDEBUG
//engine.setProgressCallback([](int progress, int left, int right, int top, int bottom) -> void {
    //    log::info("Progress: {:.2}%", (static_cast<float>(progress) / 70.0f) * 100.0f);
    //}, 10);
    //engine.setCancelCallback([](void* cancelThis, int wordCount) -> bool {
    //    return false;
    //});
#endif