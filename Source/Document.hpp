#pragma once

#include "Confidence.hpp"
#include "Core/Log.hpp"
#include "Core/Quad.hpp"

#include <optional>
#include <vector>

namespace frog {

struct Font {
    std::string name;
    int size{};
};

struct Variant {
    std::string text;
    Confidence confidence;
};

struct Symbol {
    int x{};
    int y{};
    int width{};
    int height{};
    std::string text;
    Confidence confidence;
    std::vector<Variant> variants;
};

struct Word {
    int x{};
    int y{};
    int width{};
    int height{};
    float angleInDegrees{};
    std::vector<Symbol> symbols;
    std::string text;
    Confidence confidence;
    std::optional<int> styleRefs;
};

inline Quad make_word_quad(const Word& word) {
    return make_box_quad(static_cast<float>(word.x), static_cast<float>(word.y), static_cast<float>(word.width), static_cast<float>(word.height));
}

struct Line {
    int x{};
    int y{};
    int width{};
    int height{};
    std::vector<Word> words;
    Confidence confidence;
    std::optional<int> styleRefs;
};

struct Paragraph {
    int x{};
    int y{};
    int width{};
    int height{};
    float angleInDegrees{};
    std::vector<Line> lines;
    Confidence confidence;
};

struct Block {
    int x{};
    int y{};
    int width{};
    int height{};
    std::vector<Paragraph> paragraphs;
    Confidence confidence;
    std::string detector;
    std::string recognizer;
};

struct Document {
    std::string language;
    int physicalImageNumber{};
    float rotationInDegrees{};

    std::vector<Font> fonts;
    std::vector<Block> blocks;
    Confidence confidence;

    Document() = default;
    Document(Document&&) = default;
    Document(const Document&) = delete;

    Document& operator=(Document&&) = default;
    Document& operator=(const Document&) = delete;

    void merge(const Document& that) {
        // TODO: Handle fonts.
        for (const auto& block : that.blocks) {
            blocks.push_back(block);
        }
        confidence = { (confidence.getNormalized() + that.confidence.getNormalized()) / 2.0f, Confidence::Format::normalized };
    }

};

}
