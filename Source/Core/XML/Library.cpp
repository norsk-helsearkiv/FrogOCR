#include "Library.hpp"
#include "Core/String.hpp"

#include <libxml/xmlreader.h>
#include <libxml/parserInternals.h>

namespace frog::xml::library {

static std::vector<std::function<std::optional<std::filesystem::path>(std::string_view url)>> externalToLocalResolvers;
static xmlExternalEntityLoader defaultLoader{ nullptr };

xmlParserInputPtr loadExternalEntityLocally(const char* url, const char* id, xmlParserCtxtPtr parser_context) {
    const std::string_view url_view{ url };
    if (url_view.empty()) {
        return defaultLoader ? defaultLoader(url, id, parser_context) : nullptr;
    } else {
        for (const auto& resolver : externalToLocalResolvers) {
            if (const auto path = resolver(url_view)) {
                if (auto result = xmlNewInputFromFile(parser_context, path_to_string(path.value()).c_str())) {
                    return result;
                }
            }
        }
    }
    return nullptr;
}

void initialize() {
    LIBXML_TEST_VERSION;
    defaultLoader = xmlGetExternalEntityLoader();
    xmlSetExternalEntityLoader(loadExternalEntityLocally);
}

void cleanup() {
    xmlCleanupParser();
    defaultLoader = nullptr;
    externalToLocalResolvers.clear();
}

void resolveExternalResourceToLocalFile(std::function<std::optional<std::filesystem::path>(std::string_view)> resolver) {
    externalToLocalResolvers.emplace_back(std::move(resolver));
}

}
