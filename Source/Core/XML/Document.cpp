#include "Document.hpp"
#include "Node.hpp"
#include "Core/String.hpp"

#include <libxml/xmlreader.h>

namespace frog::xml {

Document::Document(std::string_view xml) {
	document = xmlReadMemory(xml.data(), static_cast<int>(xml.size()), nullptr, nullptr, 0);
}

Document::~Document() {
	xmlFreeDoc(reinterpret_cast<xmlDoc*>(document));
}

Node Document::getRootNode() {
	return Node{ xmlDocGetRootElement(reinterpret_cast<xmlDoc*>(document)) };
}

}
