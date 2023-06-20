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

Document scan(Engine& engine, const preprocess::Image& image, const Settings& settings);

void on_block(Document& document, tesseract::ResultIterator* blockIterator);
void on_paragraph(Block& block, std::vector<std::pair<std::string, int>>& fonts, tesseract::ResultIterator* paragraphIterator);
void on_line(Paragraph& paragraph, std::vector<std::pair<std::string, int>>& fonts, tesseract::ResultIterator* lineIterator);
void on_word(Line& line, std::vector<std::pair<std::string, int>>& fonts, tesseract::ResultIterator* wordIterator);
void on_symbol(Word& word, tesseract::ResultIterator* symbolIterator);

}
