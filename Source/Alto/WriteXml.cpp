#include "WriteXml.hpp"
#include "Core/Formatting.hpp"
#include "ProcessingSoftware.hpp"
#include "TextLine.hpp"
#include "Processing.hpp"
#include "Layout.hpp"
#include "Styles.hpp"
#include "SourceImageInformation.hpp"
#include "Description.hpp"
#include "Alto.hpp"

namespace frog::alto {

void append_processing_software_xml(std::string& xml, const ProcessingSoftware& processingSoftware) {
    xml += "\t\t\t<processingSoftware>\n";
    if (!processingSoftware.softwareCreator.empty()) {
        xml += "\t\t\t\t<softwareCreator>" + processingSoftware.softwareCreator + "</softwareCreator>\n";
    }
    if (!processingSoftware.softwareName.empty()) {
        xml += "\t\t\t\t<softwareName>" + processingSoftware.softwareName + "</softwareName>\n";
    }
    if (!processingSoftware.softwareVersion.empty()) {
        xml += "\t\t\t\t<softwareVersion>" + processingSoftware.softwareVersion + "</softwareVersion>\n";
    }
    if (!processingSoftware.applicationDescription.empty()) {
        xml += "\t\t\t\t<applicationDescription>" + processingSoftware.applicationDescription + "</applicationDescription>\n";
    }
    xml += "\t\t\t</processingSoftware>\n";
}

void append_processing_xml(std::string& xml, std::string_view id, const Processing& processing) {
    xml += fmt::format("\t\t<Processing ID=\"{}\">\n", id);
    if (processing.processingCategory.has_value()) {
        std::string processingCategory;
        switch (processing.processingCategory.value()) {
        case ProcessingCategory::content_generation:
            processingCategory = "contentGeneration";
            break;
        case ProcessingCategory::content_modification:
            processingCategory = "contentModification";
            break;
        case ProcessingCategory::pre_operation:
            processingCategory = "preOperation";
            break;
        case ProcessingCategory::post_operation:
            processingCategory = "postOperation";
            break;
        case ProcessingCategory::other:
            processingCategory = "other";
            break;
        }
        xml += fmt::format("\t\t\t<processingCategory>{}</processingCategory>\n", processingCategory);
    }
    if (!processing.processingDateTime.empty()) {
        xml += fmt::format("\t\t\t<processingDateTime>{}</processingDateTime>\n", processing.processingDateTime);
    }
    if (!processing.processingAgency.empty()) {
        xml += fmt::format("\t\t\t<processingAgency>{}</processingAgency>\n", processing.processingAgency);
    }
    if (!processing.processingStepDescription.empty()) {
        xml += fmt::format("\t\t\t<processingStepDescription>{}</processingStepDescription>\n", processing.processingStepDescription);
    }
    xml += "\t\t\t<processingStepSettings>\n";
    for (const auto& processingStepSettings : processing.processingStepSettings) {
        xml += fmt::format("\t\t\t\t{}\n", processingStepSettings);
    }
    xml += "\t\t\t</processingStepSettings>\n";
    append_processing_software_xml(xml, processing.processingSoftware);
    xml += "\t\t</Processing>\n";
}

void append_text_line_xml(std::string& xml, std::string_view id, const TextLine& textLine) {
    if (textLine.strings.empty()) {
        return;
    }
    xml += fmt::format("\t\t\t\t\t\t<TextLine ID=\"{}\" HPOS=\"{}\" VPOS=\"{}\" WIDTH=\"{}\" HEIGHT=\"{}\"", id, textLine.hpos, textLine.vpos, textLine.width, textLine.height);
    if (textLine.styleRefs.has_value()) {
        xml += fmt::format(" STYLEREFS=\"textstyle_{}\"", textLine.styleRefs.value());
    }
    if (textLine.rotation != 0.0f) {
        xml += fmt::format(" ROTATION=\"{}\"", textLine.rotation);
    }
    xml += ">\n";
    int index{ 0 };
    for (const auto& string: textLine.strings) {
        append_string_xml(xml, fmt::format("{}_s_{}", id, index), string);
        index++;
    }
    xml += "\t\t\t\t\t\t</TextLine>\n";
}

void append_string_xml(std::string& xml, std::string_view id, const String& string) {
    if (string.content.empty() || string.content == " ") {
        return;
    }
    xml += fmt::format("\t\t\t\t\t\t\t<String ID=\"{}\" HPOS=\"{}\" VPOS=\"{}\" WIDTH=\"{}\" HEIGHT=\"{}\" WC=\"{:.2}\" CONTENT=\"{}\"", id, string.hpos, string.vpos, string.width, string.height, string.confidence, to_xml_attribute(string.content));
    if (string.styleRefs.has_value()) {
        xml += fmt::format(" STYLEREFS=\"textstyle_{}\"", string.styleRefs.value());
    }
    if (string.rotation != 0.0f) {
        xml += fmt::format(" ROTATION=\"{}\"", string.rotation);
    }
    if (string.glyphs.empty()) {
        xml += "/>\n";
    } else {
        xml += ">\n";
        int index{ 0 };
        for (const auto& glyph: string.glyphs) {
            append_glyph_xml(xml, fmt::format("{}_g_{}", id, index), glyph);
            index++;
        }
        xml += "\t\t\t\t\t\t\t</String>\n";
    }
}

void append_variant_xml(std::string& xml, const Variant& variant) {
    xml += fmt::format("\t\t\t\t\t\t\t\t\t<Variant CONTENT=\"{}\" VC=\"{:.2}\"/>\n", to_xml_attribute(variant.content), variant.confidence);
}

void append_glyph_xml(std::string& xml, std::string_view id, const Glyph& glyph) {
    xml += fmt::format("\t\t\t\t\t\t\t\t<Glyph ID=\"{}\" CONTENT=\"{}\" HPOS=\"{}\" VPOS=\"{}\" WIDTH=\"{}\" HEIGHT=\"{}\" GC=\"{:.2}\"", id, to_xml_attribute(glyph.content), glyph.hpos, glyph.vpos, glyph.width, glyph.height, glyph.confidence);
    if (glyph.variants.empty()) {
        xml += "/>\n";
    } else {
        xml += ">\n";
        for (const auto& variant: glyph.variants) {
            append_variant_xml(xml, variant);
        }
        xml += "\t\t\t\t\t\t\t\t</Glyph>\n";
    }
}

void append_layout_xml(std::string& xml, const Layout& layout) {
    xml += "\t<Layout>\n";
    int index{};
    for (const auto& page: layout.pages) {
        append_page_xml(xml, fmt::format("p_{}", index), page);
        index++;
    }
    xml += "\t</Layout>\n";
}

void append_source_image_information_xml(std::string& xml, const SourceImageInformation& sourceImageInformation) {
    auto fileNameString = path_to_string(sourceImageInformation.fileName);
    replace_substring(fileNameString, "\\", "/");
    xml += "\t\t<sourceImageInformation>\n";
    xml += fmt::format("\t\t\t<fileName>{}</fileName>\n", fileNameString);
    xml += "\t\t</sourceImageInformation>\n";
}

void append_description_xml(std::string& xml, const Description& description) {
    xml += "\t<Description>\n";
    append_measurement_unit_xml(xml, description.measurementUnit);
    append_source_image_information_xml(xml, description.sourceImageInformation);
    int index{};
    for (const auto& processing: description.processings) {
        append_processing_xml(xml, fmt::format("processing_{}", index), processing);
        index++;
    }
    xml += "\t</Description>\n";
}

void append_text_style_xml(std::string& xml, std::string_view id, const TextStyle& textStyle) {
    xml += fmt::format("\t\t<TextStyle ID=\"{}\" FONTFAMILY=\"{}\" FONTSIZE=\"{}\"/>\n", id, textStyle.fontFamily, textStyle.fontSize);
}

void append_page_xml(std::string& xml, std::string_view id, const Page& page) {
    xml += fmt::format("\t\t<Page ID=\"{}\" WIDTH=\"{}\" HEIGHT=\"{}\" PHYSICAL_IMG_NR=\"{}\" PC=\"{:.2}\">\n", id, page.width, page.height, page.physicalImageNumber, page.confidence);
    int index{};
    for (const auto& printSpace: page.printSpaces) {
        append_print_space_xml(xml, fmt::format("{}_ps_{}", id, index), printSpace);
        index++;
    }
    xml += "\t\t</Page>\n";
}

void append_styles_xml(std::string& xml, const Styles& styles) {
    xml += "\t<Styles>\n";
    int index{};
    for (const auto& textStyle: styles.textStyles) {
        append_text_style_xml(xml, fmt::format("textstyle_{}", index), textStyle);
        index++;
    }
    xml += "\t</Styles>\n";
}

void append_print_space_xml(std::string& xml, std::string_view id, const PrintSpace& printSpace) {
    xml += fmt::format("\t\t\t<PrintSpace ID=\"{}\" HPOS=\"{}\" VPOS=\"{}\" WIDTH=\"{}\" HEIGHT=\"{}\">\n", id, printSpace.hpos, printSpace.vpos, printSpace.width, printSpace.height);
    int index{};
    for (const auto& composedBlock: printSpace.composedBlocks) {
        append_composed_block_xml(xml, fmt::format("{}_cb_{}", id, index), composedBlock);
        index++;
    }
    xml += "\t\t\t</PrintSpace>\n";
}

void append_composed_block_xml(std::string& xml, std::string_view id, const ComposedBlock& composedBlock) {
    xml += fmt::format("\t\t\t\t<ComposedBlock ID=\"{}\" HPOS=\"{}\" VPOS=\"{}\" WIDTH=\"{}\" HEIGHT=\"{}\">\n", id, composedBlock.hpos, composedBlock.vpos, composedBlock.width, composedBlock.height);
    int index{ 0 };
    for (const auto& textBlock: composedBlock.textBlocks) {
        append_text_block_xml(xml, fmt::format("{}_tb_{}", id, index), textBlock);
        index++;
    }
    xml += "\t\t\t\t</ComposedBlock>\n";
}

std::string to_xml(const Alto& alto) {
    constexpr std::string_view xml_tag{ "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" };
    constexpr std::string_view alto_tag{
        "<alto xmlns=\"http://www.loc.gov/standards/alto/ns-v4#\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://www.loc.gov/standards/alto/ns-v4# http://www.loc.gov/standards/alto/alto-4-2.xsd\">\n"
    };
    std::string xml;
    xml.reserve(100000);
    xml += xml_tag;
    xml += alto_tag;
    append_description_xml(xml, alto.description);
    append_styles_xml(xml, alto.styles);
    append_layout_xml(xml, alto.layout);
    xml += "</alto>\n";
    return xml;
}

void append_measurement_unit_xml(std::string& xml, const MeasurementUnit& measurementUnit) {
    xml += fmt::format("\t\t<MeasurementUnit>{}</MeasurementUnit>\n", measurementUnit.type);
}

void append_text_block_xml(std::string& xml, std::string_view id, const TextBlock& textBlock) {
    if (textBlock.textLines.empty()) {
        return;
    }
    xml += fmt::format("\t\t\t\t\t<TextBlock ID=\"{}\" HPOS=\"{}\" VPOS=\"{}\" WIDTH=\"{}\" HEIGHT=\"{}\">\n", id, textBlock.hpos, textBlock.vpos, textBlock.width, textBlock.height);
    int index{ 0 };
    for (const auto& textLine: textBlock.textLines) {
        append_text_line_xml(xml, fmt::format("{}_l_{}", id, index), textLine);
        index++;
    }
    xml += "\t\t\t\t\t</TextBlock>\n";
}

}
