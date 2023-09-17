#include "Scan.hpp"
#include "Settings.hpp"
#include "Engine.hpp"
#include "PreProcess/Image.hpp"
#include "TextDetection.hpp"

#include <tesseract/baseapi.h>

namespace frog::ocr {

Document scan(Engine& engine, const preprocess::Image& image, const Settings& settings, const std::optional<std::filesystem::path>& python, const std::optional<std::filesystem::path>& script) {
    engine.setImage(image);
    engine.setSauvolaKFactor(settings.sauvolaKFactor);
    engine.setPageSegmentationMode(settings.pageSegmentation);
    engine.setCharacterWhitelist(settings.characterWhitelist);

    int x{};
    int y{};
    int width{ image.getWidth() };
    int height{ image.getHeight() };
    if (settings.cropX.has_value()) {
        x = static_cast<int>((static_cast<float>(settings.cropX.value()) / 100.0f) * static_cast<float>(image.getWidth()));
    }
    if (settings.cropY.has_value()) {
        y = static_cast<int>((static_cast<float>(settings.cropY.value()) / 100.0f) * static_cast<float>(image.getHeight()));
    }
    if (settings.cropWidth.has_value()) {
        width = static_cast<int>((static_cast<float>(settings.cropWidth.value()) / 100.0f) * static_cast<float>(image.getWidth()));
    }
    if (settings.cropHeight.has_value()) {
        height = static_cast<int>((static_cast<float>(settings.cropHeight.value()) / 100.0f) * static_cast<float>(image.getHeight()));
    }

    Document document;
    BuildState buildState;

    if (settings.textDetector == "Tesseract") {
        engine.setRectangle(x, y, width, height);
        engine.recognizeAll();
        auto resultIterator = engine.getResultIterator();
        if (!resultIterator) {
            return {};
        }
        document.confidence = engine.getConfidence();
        do {
            on_block(document, buildState, resultIterator);
        } while (resultIterator->Next(tesseract::RIL_BLOCK));
    } else if (settings.textDetector == "Paddle") {
        if (!python.has_value()) {
            log::error("Python is not configured. Aborting OCR task.");
            return {};
        }
        if (!script.has_value()) {
            log::error("PaddleFrog is not configured. Aborting OCR task.");
            return {};
        }
        if (!std::filesystem::exists(python.value())) {
            log::error("Unable to find Python at configured path: {}", python.value());
            return {};
        }
        if (!std::filesystem::exists(script.value())) {
            log::error("Unable to find PaddleFrog at configured path: {}", script.value());
            return {};
        }
        float confidence{};
        int boxCount{};
        for (const auto quad : detect_text_quads(python.value(), script.value(), image.getPath())) {
            //const auto maxDim = 0.5f * (std::abs(quad.width() * std::cos(quad.bottomRightToLeftAngle())) + std::abs(quad.height() * std::sin(quad.bottomRightToLeftAngle())));
            //const auto newWidth = static_cast<int>(maxDim) * 2;
            //const auto newHeight = static_cast<int>(maxDim) * 2;

            auto clippedPix = copy_pixels_in_quad(image.getPix(), quad);

            auto rotatedPix = pixRotate(clippedPix, -quad.bottomRightToLeftAngle(), L_ROTATE_AREA_MAP, L_BRING_IN_WHITE, 0, 0);

            const auto fnamec = fmt::format("r-{}-c.png", boxCount);
            const auto fnamer = fmt::format("r-{}-r.jpg", boxCount);
            pixWrite(fnamec.c_str(), clippedPix, IFF_PNG);
            pixWrite(fnamer.c_str(), rotatedPix, IFF_PNG);
            const preprocess::Image finalImage{ rotatedPix };
            engine.setImage(finalImage);
            engine.recognizeAll();
            if (auto resultIterator = engine.getResultIterator()) {
                confidence += engine.getConfidence().getNormalized();
                boxCount++;
                buildState.offsetX = static_cast<int>(quad.left());
                buildState.offsetY = static_cast<int>(quad.top());
                buildState.lineAngleInDegrees = quad.bottomRightToLeftAngle() * 180.0f / M_PIf;
                do {
                    on_block(document, buildState, resultIterator);
                } while (resultIterator->Next(tesseract::RIL_BLOCK));
            }
            pixDestroy(&clippedPix);
        }
        if (boxCount > 0) {
            confidence /= static_cast<float>(boxCount);
        }
        document.confidence = { confidence, Confidence::Format::normalized };
    }

    return document;

#ifndef NDEBUG
    //engine.setProgressCallback([](int progress, int left, int right, int top, int bottom) -> void {
    //    log::info("Progress: {:.2}%", (static_cast<float>(progress) / 70.0f) * 100.0f);
    //}, 10);
    //engine.setCancelCallback([](void* cancelThis, int wordCount) -> bool {
    //    return false;
    //});
#endif
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

void on_paragraph(Block& block, BuildState buildState, std::vector<std::pair<std::string, int>>& fonts, tesseract::ResultIterator* paragraphIterator) {
    Paragraph paragraph;
    paragraphIterator->BoundingBox(tesseract::RIL_PARA, &paragraph.x, &paragraph.y, &paragraph.width, &paragraph.height);
    paragraph.width -= paragraph.x;
    paragraph.height -= paragraph.y;
    paragraph.x += buildState.offsetX;
    paragraph.y += buildState.offsetY;
    auto lineIterator = paragraphIterator;
    do {
        on_line(paragraph, buildState, fonts, lineIterator);
        if (lineIterator->IsAtFinalElement(tesseract::RIL_PARA, tesseract::RIL_TEXTLINE)) {
            break;
        }
    } while (lineIterator->Next(tesseract::RIL_TEXTLINE));
    block.paragraphs.push_back(paragraph);
}

void on_line(Paragraph& paragraph, BuildState buildState, std::vector<std::pair<std::string, int>>& fonts, tesseract::ResultIterator* lineIterator) {
    Line line;
    lineIterator->BoundingBox(tesseract::RIL_TEXTLINE, &line.x, &line.y, &line.width, &line.height);
    line.width -= line.x;
    line.height -= line.y;
    line.x += buildState.offsetX;
    line.y += buildState.offsetY;
    line.angleInDegrees = buildState.lineAngleInDegrees;
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

void on_word(Line& line, BuildState buildState, std::vector<std::pair<std::string, int>>& fonts, tesseract::ResultIterator* wordIterator) {
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
        fonts.emplace_back(wordFontName, wordFontSize);
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
            variant.confidence = { symbolChoiceIterator.Confidence(), Confidence::Format::normalized };
            symbol.variants.push_back(variant);
        }
    } while (symbolChoiceIterator.Next());
    delete[] text;
    word.symbols.push_back(symbol);
}

}
