#include "Alto.hpp"
#include "Application.hpp"
#include "OCR/Document.hpp"
#include "PreProcess/Image.hpp"
#include "OCR/Settings.hpp"
#include "LoadFromXmlNode.hpp"

std::ostream& operator<<(std::ostream& out, frog::ocr::PageSegmentation pageSegmentation) {
    switch (pageSegmentation) {
        using frog::ocr::PageSegmentation;
    case PageSegmentation::automatic: return out << "Automatic";
    case PageSegmentation::sparse: return out << "Sparse";
    case PageSegmentation::block: return out << "Block";
    case PageSegmentation::line: return out << "Line";
    case PageSegmentation::word: return out << "Word";
    default: return out;
    }
}

namespace frog::alto {

Alto::Alto(const std::filesystem::path& path) {
    xml::Document document{ read_file(path) };
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

Alto::Alto(const ocr::Document& document, const preprocess::Image& image, const ocr::Settings& settings) {
    Processing initialOcrProcessing;
    initialOcrProcessing.processingCategory = ProcessingCategory::content_generation;
    initialOcrProcessing.processingDateTime = fmt::format("{}T{}", current_date_string(), current_time_string());
    initialOcrProcessing.processingAgency = application::about::creator;
    initialOcrProcessing.processingStepDescription = "OCR";
    initialOcrProcessing.processingStepSettings.emplace_back(fmt::format("SauvolaKFactor: {}", settings.sauvolaKFactor));
    std::stringstream pageSegmentation;
    pageSegmentation << settings.pageSegmentation;
    initialOcrProcessing.processingStepSettings.emplace_back(fmt::format("PageSegmentation: {}", pageSegmentation.str()));
    if (settings.cropX.has_value()) {
        initialOcrProcessing.processingStepSettings.emplace_back(fmt::format("CropX: {}%", settings.cropX.value()));
    }
    if (settings.cropY.has_value()) {
        initialOcrProcessing.processingStepSettings.emplace_back(fmt::format("CropY: {}%", settings.cropY.value()));
    }
    if (settings.cropWidth.has_value()) {
        initialOcrProcessing.processingStepSettings.emplace_back(fmt::format("CropWidth: {}%", settings.cropWidth.value()));
    }
    if (settings.cropHeight.has_value()) {
        initialOcrProcessing.processingStepSettings.emplace_back(fmt::format("CropHeight: {}%", settings.cropHeight.value()));
    }
    if (settings.minWordConfidence.has_value()) {
        initialOcrProcessing.processingStepSettings.emplace_back(fmt::format("MinWordConfidence: {}", settings.minWordConfidence.value()));
    }
    if (!settings.characterWhitelist.empty()) {
        initialOcrProcessing.processingStepSettings.emplace_back(fmt::format("CharacterWhitelist: {}", settings.characterWhitelist));
    }
    initialOcrProcessing.processingSoftware = {
        application::about::creator,
        application::about::name,
        application::version_with_build_date(),
        fmt::format("Built with Tesseract {}", tesseract::TessBaseAPI::Version())
    };

    description.sourceImageInformation.fileName = image.getPath();
    description.processings.push_back(initialOcrProcessing);

    for (const auto& [fontName, fontSize] : document.fonts) {
        styles.textStyles.emplace_back(fontName, fontSize);
    }

    Page page{ image.getWidth(), image.getHeight(), 0, document.confidence.getNormalized() };
    PrintSpace printSpace{ 0, 0, image.getWidth(), image.getHeight() };
    for (const auto& block : document.blocks) {
        ComposedBlock composedBlock;
        for (const auto& paragraph : block.paragraphs) {
            TextBlock textBlock;
            for (const auto& line : paragraph.lines) {
                TextLine textLine;
                textLine.styleRefs = line.styleRefs;
                textLine.rotation = line.angleInDegrees;
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
