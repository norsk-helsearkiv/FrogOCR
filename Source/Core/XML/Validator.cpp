#include "Validator.hpp"
#include "Core/Filesystem.hpp"
#include "Core/Log.hpp"

#include <libxml/xmlreader.h>

namespace frog::xml {

static void handle_parse_error(void* arg, xmlErrorPtr xml_error) {
    log::error("Error at line {}, column {}\n{}", xml_error->line, xml_error->int2, xml_error->message);
    if (arg) {
        *reinterpret_cast<bool*>(arg) = true;
    }
}

Validator::Validator(std::filesystem::path schema_path_) : schema_path{ std::move(schema_path_) } {
    const auto xsd = read_file(schema_path);
    if (auto context = xmlSchemaNewMemParserCtxt(xsd.c_str(), static_cast<int>(xsd.size()))) {
        schema_handle = xmlSchemaParse(context);
        xmlSchemaFreeParserCtxt(context);
        valid_schema_context_handle = xmlSchemaNewValidCtxt(reinterpret_cast<xmlSchemaPtr>(schema_handle));
    }
}

Validator::~Validator() {
    xmlSchemaFree(reinterpret_cast<xmlSchemaPtr>(schema_handle));
    xmlSchemaFreeValidCtxt(reinterpret_cast<xmlSchemaValidCtxtPtr>(valid_schema_context_handle));
}

std::optional<std::string> Validator::validate(std::string_view xml) const {
    if (auto xml_reader = xmlReaderForMemory(xml.data(), static_cast<int>(xml.size()), nullptr, nullptr, 0)) {
        auto valid_schema_context = reinterpret_cast<xmlSchemaValidCtxtPtr>(valid_schema_context_handle);
        xmlTextReaderSchemaValidateCtxt(xml_reader, valid_schema_context, 0);
        int schema_errors{ 0 };
        xmlSchemaSetValidStructuredErrors(valid_schema_context, handle_parse_error, &schema_errors);
        auto read_status = xmlTextReaderRead(xml_reader);
        while (read_status == 1 && schema_errors == 0) {
            read_status = xmlTextReaderRead(xml_reader);
        }
        std::optional<std::string> result;
        if (read_status != 0) {
            auto xml_error = xmlGetLastError();
            result = fmt::format("XML Validate error: Failed to parse in line {}, column {}. Error {}: {}", xml_error->line, xml_error->int2, xml_error->code, xml_error->message);
        }
        xmlFreeTextReader(xml_reader);
        return result;
    } else {
        return std::string{ "Failed to initialize XML reader." };
    }
}

}
