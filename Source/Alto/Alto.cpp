#include "Alto.hpp"
#include "Application.hpp"
#include "Document.hpp"
#include "Settings.hpp"
#include "Image.hpp"
#include "LoadFromXmlNode.hpp"

namespace frog::alto {

static std::string_view tesseractPageSegmentationString(PageSegmentation pageSegmentation) {
    switch (pageSegmentation) {
        using frog::PageSegmentation;
    case PageSegmentation::automatic: return "Automatic";
    case PageSegmentation::sparse: return "Sparse";
    case PageSegmentation::block: return "Block";
    case PageSegmentation::line: return "Line";
    case PageSegmentation::word: return "Word";
    default: return {};
    }
}

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

Alto::Alto(const Document& document, const Image& image, const Settings& settings) {
    Processing initialOcrProcessing;
    initialOcrProcessing.processingCategory = ProcessingCategory::content_generation;
    initialOcrProcessing.processingDateTime = fmt::format("{}T{}", current_date_string(), current_time_string());
    initialOcrProcessing.processingAgency = about::creator;
    initialOcrProcessing.processingStepDescription = "OCR";
    initialOcrProcessing.processingStepSettings.emplace_back(fmt::format("SauvolaKFactor: {}", settings.recognition.sauvolaKFactor));
    initialOcrProcessing.processingStepSettings.emplace_back(fmt::format("PageSegmentation: {}", tesseractPageSegmentationString(settings.recognition.pageSegmentation)));
    if (settings.detection.cropX.has_value()) {
        initialOcrProcessing.processingStepSettings.emplace_back(fmt::format("CropX: {}%", settings.detection.cropX.value()));
    }
    if (settings.detection.cropY.has_value()) {
        initialOcrProcessing.processingStepSettings.emplace_back(fmt::format("CropY: {}%", settings.detection.cropY.value()));
    }
    if (settings.detection.cropWidth.has_value()) {
        initialOcrProcessing.processingStepSettings.emplace_back(fmt::format("CropWidth: {}%", settings.detection.cropWidth.value()));
    }
    if (settings.detection.cropHeight.has_value()) {
        initialOcrProcessing.processingStepSettings.emplace_back(fmt::format("CropHeight: {}%", settings.detection.cropHeight.value()));
    }
    if (settings.recognition.minWordConfidence.has_value()) {
        initialOcrProcessing.processingStepSettings.emplace_back(fmt::format("MinWordConfidence: {}", settings.recognition.minWordConfidence.value()));
    }
    if (!settings.recognition.characterWhitelist.empty()) {
        initialOcrProcessing.processingStepSettings.emplace_back(fmt::format("CharacterWhitelist: {}", settings.recognition.characterWhitelist));
    }
    if (!settings.detection.textDetector.empty()) {
        initialOcrProcessing.processingStepSettings.emplace_back(fmt::format("TextDetector: {}", settings.detection.textDetector));
    }
    if (!settings.recognition.textRecognizer.empty()) {
        initialOcrProcessing.processingStepSettings.emplace_back(fmt::format("TextRecognizer: {}", settings.recognition.textRecognizer));
    }
    initialOcrProcessing.processingSoftware = {
        about::creator,
        about::name,
        version_with_build_date(),
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
