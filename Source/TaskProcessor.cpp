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

static alto::Processing create_alto_text_detection_processing(const TextDetectionSettings& settings, std::string_view extra) {
    alto::Processing processing;
    processing.processingCategory = alto::ProcessingCategory::pre_operation;
    processing.processingAgency = about::creator;
    processing.processingStepDescription = extra.empty() ? "TextDetection" : fmt::format("TextDetection{}", extra);
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
        processing.processingStepSettings.emplace_back(fmt::format("TextDetection{}.{}: {}", extra, key, value));
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

static alto::Processing create_alto_text_recognition_processing(const TextRecognitionSettings& settings, std::string_view extra) {
    alto::Processing processing;
    processing.processingCategory = alto::ProcessingCategory::content_generation;
    processing.processingAgency = about::creator;
    processing.processingStepDescription = extra.empty() ? "TextRecognition" : fmt::format("TextRecognition{}", extra);
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
    processings.emplace_back(create_alto_text_detection_processing(settings.detection, {}));
    if (settings.textAngleClassifier.has_value()) {
        processings.emplace_back(create_alto_text_angle_classification_processing(settings.textAngleClassifier.value()));
    }
    processings.emplace_back(create_alto_text_recognition_processing(settings.recognition, {}));
    if (settings.additionalDetection.has_value()) {
        processings.emplace_back(create_alto_text_detection_processing(settings.additionalDetection.value(), "2"));
    }
    if (settings.additionalRecognition.has_value()) {
        processings.emplace_back(create_alto_text_recognition_processing(settings.additionalRecognition.value(), "2"));
    }
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
    if (profile.huginMuninTextRecognizer.has_value()) {
        huginMuninTextRecognizer = std::make_unique<HuginMuninTextRecognizer>(profile.huginMuninTextRecognizer.value());
    }
    if (profile.huginMuninTextDetector.has_value()) {
        huginMuninTextDetector = std::make_unique<HuginMuninTextDetector>(profile.huginMuninTextDetector.value());
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

std::vector<float> getQuadConfidences(const std::vector<Quad>& quads, const Document& document) {
    std::vector<float> confidences;
    confidences.reserve(quads.size());
    for (const auto& quad : quads) {
        float sumConfidence{};
        float count{};
        for (const auto& block : document.blocks) {
            for (const auto& paragraph : block.paragraphs) {
                for (const auto& line: paragraph.lines) {
                    for (const auto& word: line.words) {
                        const auto wordQuad = make_word_quad(word);
                        if (quad.coverage(wordQuad) > 0.75f || wordQuad.coverage(quad) > 0.75f) {
                            sumConfidence += word.confidence.getNormalized();
                            count++;
                        }
                    }
                }
            }
        }
        confidences.push_back(sumConfidence / count);
    }
    return confidences;
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
    PIX* image{};
    if (task.inputPath.starts_with("smb://")) {
        const auto data = sambaClient->readFile(task.inputPath);
        release_samba_client();
        if (data) {
            image = pixReadMem(reinterpret_cast<const l_uint8*>(data->data()), data->size());
        }
    } else {
        image = pixRead(task.inputPath.c_str());
    }
    if (!image) {
        log::error("Failed to load image: %cyan{}", task.inputPath);
        return;
    }

    // Text Detection
    const auto* textDetector = getTextDetector(settings.detection.textDetector);
    const auto textDetectionDateTime = create_processing_date_time();
    const auto& quads = textDetector->detect(image, settings.detection);

    // Text Angle Classification
    const auto textAngleClassificationDateTime = create_processing_date_time();
    const auto& angles = runTextAngleClassifier(quads, settings, image);

    // Text Recognition
    Document document;
    const auto textRecognitionDateTime = create_processing_date_time();
    if (const auto* textRecognizer = getTextRecognizer(settings.recognition.textRecognizer)) {
        document = textRecognizer->recognize(image, quads, angles, settings.recognition);
    }

    for (auto& block : document.blocks) {
        block.detector = "processing_0";
        block.recognizer = "processing_2";
    }

    // Additional Text Detection
    const auto textDetection2DateTime = create_processing_date_time();
    std::string textRecognition2DateTime;
    if (settings.additionalDetection.has_value()) {
        const auto* additionalTextDetector = getTextDetector(settings.additionalDetection->textDetector);
        const auto& additionalQuads = additionalTextDetector->detect(image, settings.additionalDetection.value());

        std::vector<Quad> filteredQuads;
        const auto& quadConfidences = getQuadConfidences(additionalQuads, document);
        for (std::size_t additionalQuadIndex{}; additionalQuadIndex < additionalQuads.size(); additionalQuadIndex++) {
            if (quadConfidences[additionalQuadIndex] < 0.7f) {
                filteredQuads.push_back(additionalQuads[additionalQuadIndex]);
            }
        }

        if (!filteredQuads.empty()) {
            // Additional Text Recognition
            textRecognition2DateTime = create_processing_date_time();
            Document additionalDocument;
            if (const auto* additionalTextRecognizer = getTextRecognizer(settings.additionalRecognition->textRecognizer)) {
                std::vector<int> additionalAngles;
                additionalAngles.resize(filteredQuads.size());
                additionalDocument = additionalTextRecognizer->recognize(image, filteredQuads, additionalAngles, settings.additionalRecognition.value());
            }
            for (auto& block: additionalDocument.blocks) {
                block.detector = "processing_3";
                block.recognizer = "processing_4";
            }

            // Compare and pick best recognitions
            for (std::size_t blockIndex{}; blockIndex < document.blocks.size(); blockIndex++) {
                auto& block = document.blocks[blockIndex];
                for (std::size_t paragraphIndex{}; paragraphIndex < block.paragraphs.size(); paragraphIndex++) {
                    auto& paragraph = block.paragraphs[paragraphIndex];
                    for (std::size_t lineIndex{}; lineIndex < paragraph.lines.size(); lineIndex++) {
                        auto& line = paragraph.lines[lineIndex];
                        for (std::size_t wordIndex{}; wordIndex < line.words.size(); wordIndex++) {
                            auto& word = line.words[wordIndex];
                            if (word.confidence.getNormalized() > 0.5f) {
                                continue;
                            }
                            const auto wordQuad = make_word_quad(word);
                            for (const auto& additionalBlock : additionalDocument.blocks) {
                                for (const auto& additionalParagraph: additionalBlock.paragraphs) {
                                    for (const auto& additionalLine: additionalParagraph.lines) {
                                        for (const auto& additionalWord: additionalLine.words) {
                                            const auto additionalWordQuad = make_word_quad(additionalWord);
                                            if (wordQuad.coverage(additionalWordQuad) > 0.75f || additionalWordQuad.coverage(wordQuad) > 0.75f) {
                                                line.words.erase(line.words.begin() + static_cast<int>(wordIndex));
                                                if (wordIndex > 0) {
                                                    wordIndex--;
                                                }
                                                goto end_word_iteration;
                                            }
                                        }
                                    }
                                }
                            }
end_word_iteration:
                            {
                            }
                        }
                        if (line.words.empty()) {
                            paragraph.lines.erase(paragraph.lines.begin() + static_cast<int>(lineIndex));
                            if (lineIndex > 0) {
                                lineIndex--;
                            }
                        }
                    }
                    if (paragraph.lines.empty()) {
                        block.paragraphs.erase(block.paragraphs.begin() + static_cast<int>(paragraphIndex));
                        if (paragraphIndex > 0) {
                            paragraphIndex--;
                        }
                    }
                }
                if (block.paragraphs.empty()) {
                    document.blocks.erase(document.blocks.begin() + static_cast<int>(blockIndex));
                    if (blockIndex > 0) {
                        blockIndex--;
                    }
                }
            }

            // Merge documents
            document.merge(additionalDocument);
        }
    }

    // Create Alto
    alto::Alto alto{ document, task.inputPath, static_cast<int>(image->w), static_cast<int>(image->h) };
    alto.description.processings = create_alto_processings(settings);
    for (auto& processing: alto.description.processings) {
        if (processing.processingStepDescription == "TextDetection") {
            processing.processingDateTime = textDetectionDateTime;
        } else if (processing.processingStepDescription == "TextAngleClassification") {
            processing.processingDateTime = textAngleClassificationDateTime;
        } else if (processing.processingStepDescription == "TextRecognition") {
            processing.processingDateTime = textRecognitionDateTime;
        } else if (processing.processingStepDescription == "TextDetection2") {
            processing.processingDateTime = textDetection2DateTime;
        } else if (processing.processingStepDescription == "TextRecognition2") {
            processing.processingDateTime = textRecognition2DateTime;
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

    // Cleanup
    pixDestroy(&image);
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

const TextDetector* TaskProcessor::getTextDetector(std::string_view name) const {
    if (name == "Paddle") {
        return paddleTextDetector.get();
    } else if (name == "HuginMunin") {
        return huginMuninTextDetector.get();
    } else {
        return integratedTextDetector.get();
    }
}

const TextRecognizer* TaskProcessor::getTextRecognizer(std::string_view name) const {
    if (name == "Tesseract") {
        return tesseractTextRecognizer.get();
    } else if (name == "Paddle") {
        log::error("Paddle text recognition not yet supported.");
    } else if (name == "HuginMunin") {
        return huginMuninTextRecognizer.get();
    }
    return nullptr;
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