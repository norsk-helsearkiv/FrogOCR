#include "Tesseract/TesseractTextRecognizer.hpp"
#include "Image.hpp"

namespace frog {

struct BuildState {
    int offsetX{};
    int offsetY{};
    float lineAngleInDegrees{};
};

static constexpr std::string_view sauvolaThresholdingMethod{ "2" };

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

void setProgressCallback(std::function<void(int, int, int, int, int)> callback, int minDeltaForCallback) {
    threadLocalTesseractProgressCallback = callback;
    progressChangeRequiredToNotifyProgressCallback = minDeltaForCallback;
}

void setCancelCallback(std::function<bool(void*, int)> callback) {
    threadLocalTesseractCancelCallback = callback;
}

Confidence getConfidence(tesseract::TessBaseAPI& tesseract) {
    return { static_cast<float>(tesseract.MeanTextConf()), Confidence::Format::percent };
}

static std::string getTesseractVariable(const tesseract::TessBaseAPI& tesseract, std::string_view name) {
    std::string value;
    tesseract.GetVariableAsString(name.data(), &value);
    return value;
}

static bool setTesseractVariable(tesseract::TessBaseAPI& tesseract, std::string_view name, std::string_view value) {
#ifdef FROG_DEBUG_LOG_TESSERACT
    log::info("Changing {} from {} to {}", name, getTesseractVariable(tesseract, name), value);
#endif
    if (tesseract.SetVariable(name.data(), value.data())) {
        return true;
    }
    log::warning("Failed to set {} to {}.", name, value);
    return false;
}

void on_symbol(Word& word, BuildState buildState, tesseract::ResultIterator* symbolIterator) {
    Symbol symbol;
    symbolIterator->BoundingBox(tesseract::RIL_SYMBOL, &symbol.x, &symbol.y, &symbol.width, &symbol.height);
    symbol.width -= symbol.x;
    symbol.height -= symbol.y;
    symbol.x += buildState.offsetX;
    symbol.y += buildState.offsetY;
    auto text = symbolIterator->GetUTF8Text(tesseract::RIL_SYMBOL);
    symbol.text = text;
    symbol.confidence = { symbolIterator->Confidence(tesseract::RIL_SYMBOL), Confidence::Format::percent };
    tesseract::ChoiceIterator symbolChoiceIterator{ *symbolIterator };
    do {
        // It is correct to not free choiceText.
        if (auto choiceText = symbolChoiceIterator.GetUTF8Text(); choiceText && symbol.text != choiceText) {
            Variant variant;
            variant.text = choiceText;
            variant.confidence = { symbolChoiceIterator.Confidence(), Confidence::Format::percent };
            symbol.variants.push_back(variant);
        }
    } while (symbolChoiceIterator.Next());
    delete[] text;
    word.symbols.push_back(symbol);
}

void on_word(Line& line, BuildState buildState, std::vector<Font>& fonts, tesseract::ResultIterator* wordIterator) {
    Word word;
    wordIterator->BoundingBox(tesseract::RIL_WORD, &word.x, &word.y, &word.width, &word.height);
    word.width -= word.x;
    word.height -= word.y;
    word.x += buildState.offsetX;
    word.y += buildState.offsetY;
    auto text = wordIterator->GetUTF8Text(tesseract::RIL_WORD);
    if (!text) {
        return; // Most likely a blank image.
    }
    word.confidence = { wordIterator->Confidence(tesseract::RIL_WORD), Confidence::Format::percent };
    word.text = text;
    delete[] text;
    bool bold{ false };
    bool italic{ false };
    bool underlined{ false };
    bool monospace{ false };
    bool serif{ false };
    bool smallcaps{ false };
    int wordFontSize{};
    int fontId{};
    const char* wordFontNameResult = wordIterator->WordFontAttributes(&bold, &italic, &underlined, &monospace, &serif, &smallcaps, &wordFontSize, &fontId);
    const std::string_view wordFontName{ wordFontNameResult ? wordFontNameResult : "" };
    int styleRefs{};
    for (const auto& [fontName, fontSize]: fonts) {
        if (fontName == wordFontName && fontSize == wordFontSize) {
            word.styleRefs = styleRefs;
            break;
        }
        styleRefs++;
    }
    if (!word.styleRefs.has_value()) {
        word.styleRefs = static_cast<int>(fonts.size());
        fonts.emplace_back(Font{ std::string{ wordFontName }, wordFontSize });
    }
    auto symbolIterator = wordIterator;
    do {
        on_symbol(word, buildState, symbolIterator);
        if (symbolIterator->IsAtFinalElement(tesseract::RIL_WORD, tesseract::RIL_SYMBOL)) {
            break;
        }
    } while (symbolIterator->Next(tesseract::RIL_SYMBOL));
    line.words.push_back(word);
}

void on_line(Paragraph& paragraph, BuildState buildState, std::vector<Font>& fonts, tesseract::ResultIterator* lineIterator) {
    Line line;
    lineIterator->BoundingBox(tesseract::RIL_TEXTLINE, &line.x, &line.y, &line.width, &line.height);
    line.width -= line.x;
    line.height -= line.y;
    line.x += buildState.offsetX;
    line.y += buildState.offsetY;
    auto wordIterator = lineIterator;
    do {
        on_word(line, buildState, fonts, wordIterator);
        if (wordIterator->IsAtFinalElement(tesseract::RIL_TEXTLINE, tesseract::RIL_WORD)) {
            break;
        }
    } while (wordIterator->Next(tesseract::RIL_WORD));
    if (!line.words.empty()) {
        line.styleRefs = line.words[0].styleRefs;
    }
    paragraph.lines.push_back(line);
}

void on_paragraph(Block& block, BuildState buildState, std::vector<Font>& fonts, tesseract::ResultIterator* paragraphIterator) {
    Paragraph paragraph;
    paragraphIterator->BoundingBox(tesseract::RIL_PARA, &paragraph.x, &paragraph.y, &paragraph.width, &paragraph.height);
    paragraph.width -= paragraph.x;
    paragraph.height -= paragraph.y;
    paragraph.x += buildState.offsetX;
    paragraph.y += buildState.offsetY;
    paragraph.angleInDegrees = buildState.lineAngleInDegrees;
    auto lineIterator = paragraphIterator;
    do {
        on_line(paragraph, buildState, fonts, lineIterator);
        if (lineIterator->IsAtFinalElement(tesseract::RIL_PARA, tesseract::RIL_TEXTLINE)) {
            break;
        }
    } while (lineIterator->Next(tesseract::RIL_TEXTLINE));
    block.paragraphs.push_back(paragraph);
}

void on_block(Document& document, BuildState buildState, tesseract::ResultIterator* blockIterator) {
    Block block;
    blockIterator->BoundingBox(tesseract::RIL_BLOCK, &block.x, &block.y, &block.width, &block.height);
    block.width -= block.x;
    block.height -= block.y;
    block.x += buildState.offsetX;
    block.y += buildState.offsetY;
    auto paragraphIterator = blockIterator;
    do {
        on_paragraph(block, buildState, document.fonts, paragraphIterator);
        if (paragraphIterator->IsAtFinalElement(tesseract::RIL_BLOCK, tesseract::RIL_PARA)) {
            break;
        }
    } while (paragraphIterator->Next(tesseract::RIL_PARA));
    document.blocks.push_back(block);
}

void recognize_all(tesseract::TessBaseAPI& tesseract, PIX* pix) {
    tesseract::ETEXT_DESC monitor;
    monitor.progress_callback = threadLocalTesseractProgressCallback ? globalTesseractProgressCallback : nullptr;
    monitor.cancel = threadLocalTesseractCancelCallback ? globalTesseractCancelCallback : nullptr;
    if (pix) {
        tesseract.SetImage(pix);
    }
    if (tesseract.Recognize(&monitor) != 0) {
        log::error("Failed to recognize image with tesseract.");
    }
    threadLocalTesseractProgressCallback = {};
    threadLocalTesseractCancelCallback = {};
}

void build_from_result_iterator(Document& document, tesseract::ResultIterator* resultIterator, const Quad& quad, BuildState& buildState) {
    buildState.offsetX = static_cast<int>(quad.left());
    buildState.offsetY = static_cast<int>(quad.top());
    buildState.lineAngleInDegrees = quad.bottomRightToLeftAngle() * 180.0f / M_PIf;
    do {
        on_block(document, buildState, resultIterator);
    } while (resultIterator->Next(tesseract::RIL_BLOCK));
}

TesseractTextRecognizer::TesseractTextRecognizer(const TesseractConfig& config) {
    if (!std::filesystem::exists(config.tessdata)) {
        log::error("Did not find tessdata directory: {}", config.tessdata);
        return;
    }
    log::info("Initializing Tesseract ({})", config.tessdata);
    if (tesseract.Init(path_to_string(config.tessdata).c_str(), config.dataset.c_str(), tesseract::OcrEngineMode::OEM_LSTM_ONLY)) {
        log::error("Failed to initialize Tesseract.");
        return;
    }
    setTesseractVariable(tesseract, "thresholding_method", sauvolaThresholdingMethod);
    setTesseractVariable(tesseract, "lstm_choice_mode", "2");
    setTesseractVariable(tesseract, "tessedit_write_images", "true");
}

static std::pair<float, PIX*> test_confidence(tesseract::TessBaseAPI& tesseract, PIX* pix, int angle) {
    auto rotatedPix = pix;
    if (angle == 90) {
        rotatedPix = pixRotate90(pix, 1);
    } else if (angle == 180) {
        rotatedPix = pixRotate180(nullptr, rotatedPix);
    } else if (angle == 270) {
        rotatedPix = pixRotate90(pix, 1);
        pixRotate180(rotatedPix, rotatedPix);
    }
    recognize_all(tesseract, rotatedPix);
    return { static_cast<float>(tesseract.MeanTextConf()) / 100.0f, rotatedPix };
}

Document TesseractTextRecognizer::recognize(PIX* image, const std::vector<Quad>& quads, std::vector<int> angles, const TextRecognitionSettings& settings) const {
    Document document;
    BuildState buildState;

    setTesseractVariable(tesseract, "thresholding_kfactor", std::to_string(settings.sauvolaKFactor));
    setTesseractVariable(tesseract, "tessedit_char_whitelist", settings.characterWhitelist);

    if (settings.pageSegmentation == PageSegmentation::automatic) {
        tesseract.SetPageSegMode(tesseract::PageSegMode::PSM_AUTO);
    } else if (settings.pageSegmentation == PageSegmentation::block) {
        tesseract.SetPageSegMode(tesseract::PageSegMode::PSM_SINGLE_BLOCK);
    } else if (settings.pageSegmentation == PageSegmentation::line) {
        tesseract.SetPageSegMode(tesseract::PageSegMode::PSM_SINGLE_LINE);
    } else if (settings.pageSegmentation == PageSegmentation::word) {
        tesseract.SetPageSegMode(tesseract::PageSegMode::PSM_SINGLE_WORD);
    } else if (settings.pageSegmentation == PageSegmentation::sparse) {
        tesseract.SetPageSegMode(tesseract::PageSegMode::PSM_SPARSE_TEXT);
    }

    if (quads.size() == 1) {
        const auto& quad = quads.front();
        tesseract.SetImage(image);
        tesseract.SetRectangle(static_cast<int>(quad.left()), static_cast<int>(quad.top()), static_cast<int>(quad.width()), static_cast<int>(quad.height()));
        recognize_all(tesseract, nullptr);
        if (auto resultIterator = tesseract.GetIterator()) {
            document.confidence = getConfidence(tesseract);
            do {
                on_block(document, buildState, resultIterator);
            } while (resultIterator->Next(tesseract::RIL_BLOCK));
        }
    } else {
        std::size_t quadIndex{};
        for (const auto& quad : quads) {
            auto clippedPix = copy_pixels_in_quad(image, quad);
            auto pix = pixRotate(clippedPix, -quad.bottomRightToLeftAngle(), L_ROTATE_AREA_MAP, L_BRING_IN_WHITE, 0, 0);

            // Rotate to match predicted angle.
            if (angles[quadIndex] == 180) {
                pixRotate180(pix, pix);
            }

            // Run recognition for predicted angle, and try other rotations if confidence is bad.
            recognize_all(tesseract, pix);
            if (tesseract.MeanTextConf() < 40) {
                // Keep track of the best pix so far.
                auto bestConfidence = tesseract.MeanTextConf();
                auto bestAngle = angles[quadIndex];
                auto bestPix = pix;

                // Try 180 rotation.
                auto rotated180Pix = pixRotate180(nullptr, pix);
                recognize_all(tesseract, rotated180Pix);
                auto lastPix = rotated180Pix;
                if (tesseract.MeanTextConf() > bestConfidence + 10) {
                    bestConfidence = tesseract.MeanTextConf();
                    bestAngle = angles[quadIndex] + 180;
                    bestPix = rotated180Pix;
                }

#if 0
                // Try 90 rotation.
                PIX* rotated90Pix{};
                if (bestConfidence < 50 && bestConfidence > 1) {
                    rotated90Pix = pixRotate90(pix, 1);
                    recognize_all(tesseract, rotated90Pix);
                    lastPix = rotated90Pix;
                    if (tesseract.MeanTextConf() > bestConfidence + 30) {
                        bestConfidence = tesseract.MeanTextConf();
                        bestAngle = angles[quadIndex] + 90;
                        bestPix = rotated90Pix;
                    }
                }

                // Try 270 rotation.
                PIX* rotated270Pix{};
                if (bestConfidence < 50 && bestConfidence > 1) {
                    rotated270Pix = pixRotate90(pix, -1);
                    recognize_all(tesseract, rotated270Pix);
                    lastPix = rotated270Pix;
                    if (tesseract.MeanTextConf() > bestConfidence + 30) {
                        bestConfidence = tesseract.MeanTextConf();
                        bestAngle = angles[quadIndex] + 270;
                        bestPix = rotated270Pix;
                    }
                }
#endif

                // Rerun recognition for best angle if necessary.
                angles[quadIndex] = bestAngle % 360;
                if (lastPix != bestPix) {
                    recognize_all(tesseract, bestPix);
                }

                // Clean up
                pixDestroy(&rotated180Pix);
#if 0
                pixDestroy(&rotated90Pix);
                pixDestroy(&rotated270Pix);
#endif
            }

            // Build document.
            if (auto resultIterator = tesseract.GetIterator()) {
                build_from_result_iterator(document, resultIterator, quad, buildState);
            }

            pixDestroy(&pix);
            pixDestroy(&clippedPix);
            quadIndex++;
        }
        float confidence{};
        int wordCount{};
        for (const auto& block : document.blocks) {
            for (const auto& paragraph : block.paragraphs) {
                for (const auto& line : paragraph.lines) {
                    for (const auto& word : line.words) {
                        confidence += word.confidence.getNormalized();
                        wordCount++;
                    }
                }
            }
        }
        if (wordCount > 0) {
            confidence /= static_cast<float>(wordCount);
        }
        document.confidence = { confidence, Confidence::Format::normalized };
    }

    std::unordered_map<int, int> angleCounts;
    for (auto& angle : angles) {
        angleCounts[angle]++;
    }
    int mostUsedAngle{};
    for (const auto& [angle, count] : angleCounts) {
        if (count > angleCounts[mostUsedAngle]) {
            mostUsedAngle = angle;
        }
    }
    document.rotationInDegrees = static_cast<float>(mostUsedAngle);

    return document;
}

}
