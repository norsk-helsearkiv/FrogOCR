#include "LoadFromXmlNode.hpp"

namespace frog::alto {

ComposedBlock make_composed_block_from_xml_node(xml::Node node) {
    ComposedBlock composedBlock;
    composedBlock.id = node.getAttribute("ID");
    composedBlock.hpos = node.getIntAttribute("HPOS");
    composedBlock.vpos = node.getIntAttribute("VPOS");
    composedBlock.width = node.getIntAttribute("WIDTH");
    composedBlock.height = node.getIntAttribute("HEIGHT");
    for (const auto& textBlockNode : node.getChildren("TextBlock")) {
        composedBlock.textBlocks.emplace_back(make_text_block_from_xml_node(textBlockNode));
    }
    return composedBlock;
}

Processing make_processing_from_xml_node(xml::Node node) {
    Processing processing;
    for (auto processingSoftwareNode : node.getChildren("ProcessingSoftware")) {
        processing.processingSoftware = make_processing_software_from_xml_node(processingSoftwareNode);
    }
    return processing;
}

ProcessingSoftware make_processing_software_from_xml_node(xml::Node node) {
    ProcessingSoftware processingSoftware;
    if (const auto softwareCreatorNode = node.findFirstChild("softwareCreator")) {
        processingSoftware.softwareCreator = softwareCreatorNode->getContent();
    }
    if (const auto softwareNameNode = node.findFirstChild("softwareName")) {
        processingSoftware.softwareName = softwareNameNode->getContent();
    }
    if (const auto softwareVersionNode = node.findFirstChild("softwareVersion")) {
        processingSoftware.softwareVersion = softwareVersionNode->getContent();
    }
    if (const auto applicationDescriptionNode = node.findFirstChild("applicationDescription")) {
        processingSoftware.applicationDescription = applicationDescriptionNode->getContent();
    }
    return processingSoftware;
}

Description make_description_from_xml_node(xml::Node node) {
    Description description;
    if (auto measurementUnitNode = node.findFirstChild("MeasurementUnit")) {
        description.measurementUnit = make_measurement_unit_from_xml_node(measurementUnitNode.value());
    }
    if (auto sourceImageInformationNode = node.findFirstChild("sourceImageInformation")) {
        description.sourceImageInformation = make_source_image_information_from_xml_node(sourceImageInformationNode.value());
    }
    for (auto processing_node : node.getChildren("Processing")) {
        description.processings.emplace_back(make_processing_from_xml_node(processing_node));
    }
    return description;
}

Glyph make_glyph_from_xml_node(xml::Node node) {
    Glyph glyph;
    glyph.hpos = node.getIntAttribute("HPOS");
    glyph.vpos = node.getIntAttribute("VPOS");
    glyph.width = node.getIntAttribute("WIDTH");
    glyph.height = node.getIntAttribute("HEIGHT");
    glyph.confidence = node.getFloatAttribute("GC");
    glyph.content = node.getAttribute("CONTENT");
    for (const auto& variantNode : node.getChildren("Variant")) {
        glyph.variants.emplace_back(make_variant_from_xml_node(variantNode));
    }
    return glyph;
}

Layout make_layout_from_xml_node(xml::Node node) {
    Layout layout;
    for (const auto& pageNode : node.getChildren("Page")) {
        layout.pages.emplace_back(make_page_from_xml_node(pageNode));
    }
    return layout;
}

MeasurementUnit make_measurement_unit_from_xml_node(xml::Node node) {
    MeasurementUnit measurementUnit;
    measurementUnit.type = node.getContent();
    return measurementUnit;
}

Page make_page_from_xml_node(xml::Node node) {
    Page page;
    page.width = node.getIntAttribute("WIDTH");
    page.height = node.getIntAttribute("HEIGHT");
    page.physicalImageNumber = node.getIntAttribute("PHYSICAL_IMG_NR");
    page.confidence = node.getFloatAttribute("PC");
    for (const auto& printSpaceNode : node.getChildren("PrintSpace")) {
        page.printSpaces.emplace_back(make_print_space_from_xml_node(printSpaceNode));
    }
    return page;
}

PrintSpace make_print_space_from_xml_node(xml::Node node) {
    PrintSpace printSpace;
    printSpace.hpos = node.getIntAttribute("HPOS");
    printSpace.vpos = node.getIntAttribute("VPOS");
    printSpace.width = node.getIntAttribute("WIDTH");
    printSpace.height = node.getIntAttribute("HEIGHT");
    for (const auto& composedBlockNode : node.getChildren("ComposedBlock")) {
        printSpace.composedBlocks.emplace_back(make_composed_block_from_xml_node(composedBlockNode));
    }
    return printSpace;
}

SourceImageInformation make_source_image_information_from_xml_node(xml::Node node) {
    SourceImageInformation sourceImageInformation;
    if (auto fileNameNode = node.findFirstChild("fileName")) {
        sourceImageInformation.fileName = fileNameNode->getContent();
    }
    return sourceImageInformation;
}

String make_string_from_xml_node(xml::Node node) {
    String string;
    string.hpos = node.getIntAttribute("HPOS");
    string.vpos = node.getIntAttribute("VPOS");
    string.width = node.getIntAttribute("WIDTH");
    string.height = node.getIntAttribute("HEIGHT");
    string.confidence = node.getFloatAttribute("WC");
    string.content = node.getAttribute("CONTENT");
    for (const auto& glyphNode : node.getChildren("Glyph")) {
        string.glyphs.emplace_back(make_glyph_from_xml_node(glyphNode));
    }
    string.styleRefs = from_string<int>(node.getAttribute("STYLEREFS").substr(std::string_view{ "textstyle_" }.size()));
    return string;
}

Styles make_styles_from_xml_node(xml::Node node) {
    Styles styles;
    for (const auto& textStyleNode : node.getChildren("TextStyle")) {
        styles.textStyles.emplace_back(make_text_style_from_xml_node(textStyleNode));
    }
    return styles;
}

TextBlock make_text_block_from_xml_node(xml::Node node) {
    TextBlock textBlock;
    textBlock.hpos = node.getIntAttribute("HPOS");
    textBlock.vpos = node.getIntAttribute("VPOS");
    textBlock.width = node.getIntAttribute("WIDTH");
    textBlock.height = node.getIntAttribute("HEIGHT");
    for (const auto& textLineNode: node.getChildren("TextLine")) {
        textBlock.textLines.emplace_back(make_text_line_from_xml_node(textLineNode));
    }
    return textBlock;
}

TextLine make_text_line_from_xml_node(xml::Node node) {
    TextLine textLine;
    textLine.hpos = node.getIntAttribute("HPOS");
    textLine.vpos = node.getIntAttribute("VPOS");
    textLine.width = node.getIntAttribute("WIDTH");
    textLine.height = node.getIntAttribute("HEIGHT");
    textLine.styleRefs = from_string<int>(node.getAttribute("STYLEREFS").substr(std::string_view{ "textstyle_" }.size()));
    for (const auto& stringNode: node.getChildren("String")) {
        textLine.strings.emplace_back(make_string_from_xml_node(stringNode));
    }
    return textLine;
}

Variant make_variant_from_xml_node(xml::Node node) {
    Variant variant;
    variant.content = node.getAttribute("CONTENT");
    variant.confidence = node.getFloatAttribute("VC");
    return variant;
}

TextStyle make_text_style_from_xml_node(xml::Node node) {
    TextStyle textStyle;
    textStyle.fontFamily = node.getAttribute("FONTFAMILY");
    textStyle.fontSize = node.getIntAttribute("FONTSIZE", TextStyle::defaultFontSize);
    return textStyle;
}

}
