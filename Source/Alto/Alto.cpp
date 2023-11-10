#include "Alto.hpp"
#include "Application.hpp"
#include "Document.hpp"
#include "LoadFromXmlNode.hpp"

namespace frog::alto {

Alto::Alto(std::string_view xml) {
    xml::Document document{ xml };
    auto altoNode = document.getRootNode();
    if (auto descriptionNode = altoNode.findFirstChild("Description")) {
        description = make_description_from_xml_node(descriptionNode.value());
    }
    if (auto stylesNode = altoNode.findFirstChild("Styles")) {
        styles = make_styles_from_xml_node(stylesNode.value());
    }
    if (auto layoutNode = altoNode.findFirstChild("Layout")) {
        layout = make_layout_from_xml_node(layoutNode.value());
    }
}

Alto::Alto(const Document& document, std::string path, int width, int height) {
    description.sourceImageInformation.fileName = path;
    for (const auto& [fontName, fontSize] : document.fonts) {
        styles.textStyles.emplace_back(fontName, fontSize);
    }
    Page page{ document.language, width, height, document.physicalImageNumber, document.confidence.getNormalized(), document.rotationInDegrees };
    PrintSpace printSpace{ 0, 0, width, height };
    for (const auto& block : document.blocks) {
        ComposedBlock composedBlock;
        for (const auto& paragraph : block.paragraphs) {
            if (paragraph.lines.empty()) {
                continue;
            }
            TextBlock textBlock;
            textBlock.rotation = paragraph.angleInDegrees;
            for (const auto& line : paragraph.lines) {
                if (line.words.empty()) {
                    continue;
                }
                TextLine textLine;
                textLine.styleRefs = line.styleRefs;
                for (const auto& word: line.words) {
                    std::vector<Glyph> glyphs;
                    for (const auto& symbol : word.symbols) {
                        std::vector<Variant> variants;
                        for (const auto& variant: symbol.variants) {
                            variants.emplace_back(variant.text, variant.confidence.getNormalized());
                        }
                        glyphs.emplace_back(symbol.text, symbol.confidence.getNormalized(), symbol.x, symbol.y, symbol.width, symbol.height, variants);
                    }
                    textLine.strings.emplace_back(word.text, word.confidence.getNormalized(), word.x, word.y, word.width, word.height, word.angleInDegrees, glyphs, word.styleRefs);
                }
                textLine.setBoundingBox(line.x, line.y, line.width, line.height);
                textBlock.textLines.emplace_back(std::move(textLine));
            }
            textBlock.setBoundingBox(paragraph.x, paragraph.y, paragraph.width, paragraph.height);
            composedBlock.textBlocks.emplace_back(std::move(textBlock));
        }
        composedBlock.setBoundingBox(block.x, block.y, block.width, block.height);
        printSpace.composedBlocks.emplace_back(std::move(composedBlock));
    }
    page.printSpaces.emplace_back(std::move(printSpace));
    layout.pages.emplace_back(std::move(page));
}

}
