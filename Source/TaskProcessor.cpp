#include "TaskProcessor.hpp"
#include "Core/SambaClient.hpp"
#include "Application.hpp"

namespace frog {

static std::string get_application_description_for_alto() {
    return fmt::format("Tesseract {} / Paddle {} / OpenCV {}", get_tesseract_version(), get_paddle_version(), get_opencv_version());
}

static std::string create_processing_date_time() {
    return fmt::format("{}T{}", current_date_string(), current_time_string());
}

static alto::ProcessingSoftware create_processing_software() {
    return { about::creator, about::name, version_with_build_date(), get_application_description_for_alto() };
}

static alto::Processing create_alto_text_detection_processing(const TextDetectionSettings& settings) {
    alto::Processing processing;
    processing.processingCategory = alto::ProcessingCategory::pre_operation;
    processing.processingAgency = about::creator;
    processing.processingStepDescription = "TextDetection";
    processing.processingStepSettings.emplace_back(fmt::format("TextDetector: {}", settings.textDetector));
    if (settings.cropX.has_value()) {
        processing.processingStepSettings.emplace_back(fmt::format("CropX: {}%", settings.cropX.value()));
    }
    if (settings.cropY.has_value()) {
        processing.processingStepSettings.emplace_back(fmt::format("CropY: {}%", settings.cropY.value()));
    }
    if (settings.cropWidth.has_value()) {
        processing.processingStepSettings.emplace_back(fmt::format("CropWidth: {}%", settings.cropWidth.value()));
    }
    if (settings.cropHeight.has_value()) {
        processing.processingStepSettings.emplace_back(fmt::format("CropHeight: {}%", settings.cropHeight.value()));
    }
    for (const auto& [key, value] : settings.other) {
        processing.processingStepSettings.emplace_back(fmt::format("TextDetection.{}: {}", key, value));
    }
    processing.processingSoftware = create_processing_software();
    return processing;
}

static alto::Processing create_alto_text_angle_classification_processing(std::string textAngleClassifier) {
    alto::Processing processing;
    processing.processingCategory = alto::ProcessingCategory::pre_operation;
    processing.processingAgency = about::creator;
    processing.processingStepDescription = "TextAngleClassification";
    processing.processingStepSettings.emplace_back(fmt::format("TextAngleClassifier: {}", textAngleClassifier));
    processing.processingSoftware = create_processing_software();
    return processing;
}

static alto::Processing create_alto_text_recognition_processing(const TextRecognitionSettings& settings) {
    alto::Processing processing;
    processing.processingCategory = alto::ProcessingCategory::content_generation;
    processing.processingAgency = about::creator;
    processing.processingStepDescription = "TextRecognition";
    processing.processingStepSettings.emplace_back(fmt::format("TextRecognizer: {}", settings.textRecognizer));
    if (settings.textRecognizer == "Tesseract") {
        processing.processingStepSettings.emplace_back(fmt::format("SauvolaKFactor: {}", settings.sauvolaKFactor));
        processing.processingStepSettings.emplace_back(fmt::format("PageSegmentation: {}", page_segmentation_string(settings.pageSegmentation)));
        if (!settings.characterWhitelist.empty()) {
            processing.processingStepSettings.emplace_back(fmt::format("CharacterWhitelist: {}", settings.characterWhitelist));
        }
    }
    if (settings.minWordConfidence.has_value()) {
        processing.processingStepSettings.emplace_back(fmt::format("MinWordConfidence: {}", settings.minWordConfidence.value()));
    }
    processing.processingSoftware = create_processing_software();
    return processing;
}

static std::vector<alto::Processing> create_alto_processings(const Settings& settings) {
    std::vector<alto::Processing> processings;
    processings.emplace_back(create_alto_text_detection_processing(settings.detection));
    if (settings.textAngleClassifier.has_value()) {
        processings.emplace_back(create_alto_text_angle_classification_processing(settings.textAngleClassifier.value()));
    }
    processings.emplace_back(create_alto_text_recognition_processing(settings.recognition));
    return processings;
}

TaskProcessor::TaskProcessor(const Profile& profile) {
    integratedTextDetector = std::make_unique<IntegratedTextDetector>();
    if (profile.paddleTextDetector.has_value()) {
        paddleTextDetector = std::make_unique<PaddleTextDetector>(profile.paddleTextDetector.value());
    }
    if (profile.tesseract.has_value()) {
        tesseractTextRecognizer = std::make_unique<TesseractTextRecognizer>(profile.tesseract.value());
    }
    if (profile.paddleTextOrientationClassifier.has_value()) {
        paddleTextAngleClassifier = std::make_unique<PaddleTextAngleClassifier>(profile.paddleTextOrientationClassifier.value());
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
    log::info("{}", task.inputPath);

    const Settings settings{ task.settingsCsv };

    // Pre-checks
    SambaClient* sambaClient{};
    if (task.inputPath.starts_with("smb://")) {
        sambaClient = acquire_samba_client();
        if (!sambaClient) {
            log::error("Samba client not configured. Skipping task {}", task.inputPath);
            return;
        }
        if (!settings.overwriteOutput && sambaClient->exists(task.outputPath)) {
            release_samba_client();
            log::warning("Output file already exists: %cyan{}", task.outputPath);
            return;
        }
        if (!sambaClient->exists(task.inputPath)) {
            release_samba_client();
            log::error("Input file does not exist: %cyan{}", task.inputPath);
            return;
        }
    } else {
        if (!settings.overwriteOutput && std::filesystem::exists(task.outputPath)) {
            log::warning("Output file already exists: %cyan{}", task.outputPath);
            return;
        }
        if (!std::filesystem::exists(task.inputPath)) {
            log::error("Input file does not exist: %cyan{}", task.inputPath);
            return;
        }
    }

    // Initialize
    std::unique_ptr<Image> image;
    if (task.inputPath.starts_with("smb://")) {
        const auto data = sambaClient->readFile(task.inputPath);
        release_samba_client();
        if (data) {
            image = std::make_unique<Image>(data->data(), data->size());
        }
    } else {
        image = std::make_unique<Image>(task.inputPath);
    }
    if (!image || !image->isOk()) {
        log::error("Failed to load image: %cyan{}", task.inputPath);
        return;
    }

    TextDetector* textDetector{};
    if (settings.detection.textDetector == "Paddle") {
        textDetector = paddleTextDetector.get();
    } else {
        textDetector = integratedTextDetector.get();
    }
    if (!textDetector) {
        log::error("No text detector.");
        return;
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
    if (!textRecognizer) {
        log::error("No text recognizer.");
        return;
    }

    // Text Detection
    const auto textDetectionDateTime = create_processing_date_time();
    const auto& quads = textDetector->detect(*image, settings.detection);

    // Text Angle Classification
    const auto textAngleClassificationDateTime = create_processing_date_time();
    const auto& angles = runTextAngleClassifier(quads, settings, image->getPix());

    // Text Recognition
    const auto textRecognitionDateTime = create_processing_date_time();
    auto ocrDocument = textRecognizer->recognize(*image, quads, angles, settings.recognition);

    // Create Alto
    alto::Alto alto{ ocrDocument, task.inputPath, image->getWidth(), image->getHeight() };
    alto.description.processings = create_alto_processings(settings);
    for (auto& processing : alto.description.processings) {
        if (processing.processingStepDescription == "TextDetection") {
            processing.processingDateTime = textDetectionDateTime;
        } else if (processing.processingStepDescription == "TextAngleClassification") {
            processing.processingDateTime = textAngleClassificationDateTime;
        } else if (processing.processingStepDescription == "TextRecognition") {
            processing.processingDateTime = textRecognitionDateTime;
        } else {
            processing.processingDateTime = create_processing_date_time();
        }
    }
    const auto altoXml = alto::to_xml(alto);

    // Save
    if (task.outputPath.starts_with("smb://")) {
        sambaClient = acquire_samba_client();
        if (!sambaClient->writeFile(task.outputPath, altoXml)) {
            log::error("Failed to write AltoXML file: {}", task.outputPath);
        }
        release_samba_client();
    } else {
        if (!write_file(task.outputPath, altoXml)) {
            log::error("Failed to write AltoXML file: {}", task.outputPath);
        }
    }
}

std::vector<int> TaskProcessor::runTextAngleClassifier(const std::vector<Quad>& quads, const Settings& settings, PIX* pix) {
    std::vector<int> angles;
    angles.resize(quads.size(), 0);
    if (!paddleTextAngleClassifier) {
        return angles;
    }
    if (settings.textAngleClassifier == "Paddle") {
        const auto& classifications = paddleTextAngleClassifier->classify(pix, quads);
        if (classifications.size() != quads.size()) {
            log::error("The number of text angle classifications ({}) and quads ({}) do not match.", classifications.size(), quads.size());
            return angles;
        }
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
    return angles;
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