#pragma once

#include "Document.hpp"

namespace tesseract {
class ResultIterator;
}

namespace frog::preprocess {
class Image;
}

namespace frog::ocr {

class Engine;
class Settings;

struct BuildState {
    int offsetX{};
    int offsetY{};
    float lineAngleInDegrees{};
};

Document scan(Engine& engine, const preprocess::Image& image, const Settings& settings, const std::optional<std::filesystem::path>& python, const std::optional<std::filesystem::path>& script);

void on_block(Document& document, BuildState buildState, tesseract::ResultIterator* blockIterator);
void on_paragraph(Block& block, BuildState buildState, std::vector<std::pair<std::string, int>>& fonts, tesseract::ResultIterator* paragraphIterator);
void on_line(Paragraph& paragraph, BuildState buildState, std::vector<std::pair<std::string, int>>& fonts, tesseract::ResultIterator* lineIterator);
void on_word(Line& line, BuildState buildState, std::vector<std::pair<std::string, int>>& fonts, tesseract::ResultIterator* wordIterator);
void on_symbol(Word& word, BuildState buildState, tesseract::ResultIterator* symbolIterator);

}
