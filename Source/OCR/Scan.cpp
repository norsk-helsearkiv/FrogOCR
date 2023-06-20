#include "Scan.hpp"
#include "Settings.hpp"
#include "Engine.hpp"
#include "PreProcess/Image.hpp"

#include <tesseract/baseapi.h>

namespace frog::ocr {

Document scan(Engine& engine, const preprocess::Image& image, const Settings& settings) {
    engine.setImage(image);
    engine.setSauvolaKFactor(settings.sauvolaKFactor);
    engine.setPageSegmentationMode(settings.pageSegmentation);

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
    engine.setRectangle(x, y, width, height);
    engine.setCharacterWhitelist(settings.characterWhitelist);

#ifndef NDEBUG
    engine.setProgressCallback([](int progress, int left, int right, int top, int bottom) -> void {
        log::info("Progress: {:.2}%", (static_cast<float>(progress) / 70.0f) * 100.0f);
    }, 10);
    engine.setCancelCallback([](void* cancelThis, int wordCount) -> bool {
        return false;
    });
#endif
    engine.recognizeAll();
    auto resultIterator = engine.getResultIterator();
    if (!resultIterator) {
        return {};
    }
    Document document;
    document.confidence = engine.getConfidence();
    do {
        on_block(document, resultIterator);
    } while (resultIterator->Next(tesseract::RIL_BLOCK));
    return document;
}

void on_block(Document& document, tesseract::ResultIterator* blockIterator) {
    Block block;
    blockIterator->BoundingBox(tesseract::RIL_BLOCK, &block.x, &block.y, &block.width, &block.height);
    block.width -= block.x;
    block.height -= block.y;
    auto paragraphIterator = blockIterator;
    do {
        on_paragraph(block, document.fonts, paragraphIterator);
        if (paragraphIterator->IsAtFinalElement(tesseract::RIL_BLOCK, tesseract::RIL_PARA)) {
            break;
        }
    } while (paragraphIterator->Next(tesseract::RIL_PARA));
    document.blocks.push_back(block);
}

void on_paragraph(Block& block, std::vector<std::pair<std::string, int>>& fonts, tesseract::ResultIterator* paragraphIterator) {
    Paragraph paragraph;
    paragraphIterator->BoundingBox(tesseract::RIL_PARA, &paragraph.x, &paragraph.y, &paragraph.width, &paragraph.height);
    paragraph.width -= paragraph.x;
    paragraph.height -= paragraph.y;
    auto lineIterator = paragraphIterator;
    do {
        on_line(paragraph, fonts, lineIterator);
        if (lineIterator->IsAtFinalElement(tesseract::RIL_PARA, tesseract::RIL_TEXTLINE)) {
            break;
        }
    } while (lineIterator->Next(tesseract::RIL_TEXTLINE));
    block.paragraphs.push_back(paragraph);
}

void on_line(Paragraph& paragraph, std::vector<std::pair<std::string, int>>& fonts, tesseract::ResultIterator* lineIterator) {
    Line line;
    lineIterator->BoundingBox(tesseract::RIL_TEXTLINE, &line.x, &line.y, &line.width, &line.height);
    line.width -= line.x;
    line.height -= line.y;
    auto wordIterator = lineIterator;
    do {
        on_word(line, fonts, wordIterator);
        if (wordIterator->IsAtFinalElement(tesseract::RIL_TEXTLINE, tesseract::RIL_WORD)) {
            break;
        }
    } while (wordIterator->Next(tesseract::RIL_WORD));
    if (!line.words.empty()) {
        line.styleRefs = line.words[0].styleRefs;
    }
    paragraph.lines.push_back(line);
}

void on_word(Line& line, std::vector<std::pair<std::string, int>>& fonts, tesseract::ResultIterator* wordIterator) {
    Word word;
    wordIterator->BoundingBox(tesseract::RIL_WORD, &word.x, &word.y, &word.width, &word.height);
    word.width -= word.x;
    word.height -= word.y;
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
        on_symbol(word, symbolIterator);
        if (symbolIterator->IsAtFinalElement(tesseract::RIL_WORD, tesseract::RIL_SYMBOL)) {
            break;
        }
    } while (symbolIterator->Next(tesseract::RIL_SYMBOL));
    line.words.push_back(word);
}

void on_symbol(Word& word, tesseract::ResultIterator* symbolIterator) {
    Symbol symbol;
    symbolIterator->BoundingBox(tesseract::RIL_SYMBOL, &symbol.x, &symbol.y, &symbol.width, &symbol.height);
    symbol.width -= symbol.x;
    symbol.height -= symbol.y;
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
