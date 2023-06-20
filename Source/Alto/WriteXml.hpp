#pragma once

#include <string>

namespace frog::alto {

struct ProcessingSoftware;
struct Processing;
struct TextLine;
struct Variant;
struct Glyph;
struct Layout;
struct SourceImageInformation;
struct Description;
struct TextStyle;
struct Page;
struct Styles;
struct PrintSpace;
struct ComposedBlock;
struct String;
struct MeasurementUnit;
struct TextBlock;
struct Alto;

void append_processing_software_xml(std::string& xml, const ProcessingSoftware& processingSoftware);
void append_processing_xml(std::string& xml, std::string_view id, const Processing& processing);
void append_text_line_xml(std::string& xml, std::string_view id, const TextLine& textLine);
void append_variant_xml(std::string& xml, const Variant& variant);
void append_glyph_xml(std::string& xml, std::string_view id, const Glyph& glyph);
void append_layout_xml(std::string& xml, const Layout& layout);
void append_description_xml(std::string& xml, const Description& description);
void append_text_style_xml(std::string& xml, std::string_view id, const TextStyle& textStyle);
void append_page_xml(std::string& xml, std::string_view id, const Page& page);
void append_styles_xml(std::string& xml, const Styles& styles);
void append_print_space_xml(std::string& xml, std::string_view id, const PrintSpace& printSpace);
void append_composed_block_xml(std::string& xml, std::string_view id, const ComposedBlock& composedBlock);
void append_string_xml(std::string& xml, std::string_view id, const String& string);
void append_measurement_unit_xml(std::string& xml, const MeasurementUnit& measurementUnit);
void append_text_block_xml(std::string& xml, std::string_view id, const TextBlock& textBlock);

std::string to_xml(const Alto& alto);

}
