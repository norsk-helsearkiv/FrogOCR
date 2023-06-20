#pragma once

#include "ComposedBlock.hpp"
#include "Processing.hpp"
#include "Description.hpp"
#include "Glyph.hpp"
#include "Layout.hpp"
#include "Styles.hpp"

namespace frog::alto {

ComposedBlock make_composed_block_from_xml_node(xml::Node node);
Processing make_processing_from_xml_node(xml::Node node);
ProcessingSoftware make_processing_software_from_xml_node(xml::Node node);
Description make_description_from_xml_node(xml::Node node);
Glyph make_glyph_from_xml_node(xml::Node node);
Layout make_layout_from_xml_node(xml::Node node);
MeasurementUnit make_measurement_unit_from_xml_node(xml::Node node);
Page make_page_from_xml_node(xml::Node node);
PrintSpace make_print_space_from_xml_node(xml::Node node);
SourceImageInformation make_source_image_information_from_xml_node(xml::Node node);
Styles make_styles_from_xml_node(xml::Node node);
TextBlock make_text_block_from_xml_node(xml::Node node);
TextLine make_text_line_from_xml_node(xml::Node node);
Variant make_variant_from_xml_node(xml::Node node);
TextStyle make_text_style_from_xml_node(xml::Node node);

}
