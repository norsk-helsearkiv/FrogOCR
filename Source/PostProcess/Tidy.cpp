#include "Tidy.hpp"
#include "OCR/Document.hpp"

namespace frog::postprocess {

static bool is_garbage_word(std::string_view word) {

}

void remove_garbage(ocr::Document& document) {
    for (auto& block : document.blocks) {
        for (auto& paragraph : block.paragraphs) {
            for (auto& line : paragraph.lines) {
                for (int i{}; i < static_cast<int>(line.words.size()); i++) {
                    auto& word = line.words[i];
                    if (word.confidence.getNormalized() > 0.5f) {
                        continue;
                    }
                    if (is_garbage_word(word.text)) {
                        line.words.erase(line.words.begin() + i);
                        i--;
                    }
                }
            }
        }
    }
}

}