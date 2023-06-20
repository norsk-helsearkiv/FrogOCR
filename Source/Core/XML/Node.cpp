#include "Node.hpp"
#include "Core/String.hpp"

#include <libxml/xmlreader.h>

namespace frog::xml {

Node::Node(void* handle) : handle{ handle } {

}

std::string_view Node::getName() const {
    if (!handle) {
        return {};
    }
    auto node = reinterpret_cast<xmlNode*>(handle);
    return node->name ? reinterpret_cast<const char*>(node->name) : std::string_view{};
}

std::vector<Node> Node::getChildren(std::string_view tagName) const {
    if (!handle) {
        return {};
    }
    auto node = reinterpret_cast<xmlNode*>(handle);
    std::vector<Node> children;
    for (node = node->children; node; node = node->next) {
        if (node->type == XML_ELEMENT_NODE) {
            if (tagName.empty() || reinterpret_cast<const char*>(node->name) == tagName) {
                children.push_back(node);
            }
        }
    }
    return children;
}

std::optional<Node> Node::findFirstChild(std::string_view tagName) const {
    if (!handle) {
        return std::nullopt;
    }
    auto node = reinterpret_cast<xmlNode*>(handle);
    for (node = node->children; node; node = node->next) {
        if (node->type == XML_ELEMENT_NODE) {
            if (tagName.empty() || reinterpret_cast<const char*>(node->name) == tagName) {
                return Node{ node };
            }
        }
    }
    return std::nullopt;
}

std::optional<std::string> Node::findAttribute(std::string_view name) const {
    if (!handle) {
        return std::nullopt;
    }
    auto node = reinterpret_cast<xmlNode*>(handle);
    auto attribute = xmlGetProp(node, reinterpret_cast<const xmlChar*>(name.data()));
    if (!attribute) {
        return std::nullopt;
    }
    return reinterpret_cast<const char*>(attribute);
}

std::string Node::getAttribute(std::string_view name, std::string fallback) const {
    return findAttribute(name).value_or(fallback);
}

int Node::getIntAttribute(std::string_view name, int fallback) const {
    if (const auto attribute = findAttribute(name)) {
        return from_string<int>(attribute.value()).value_or(fallback);
    } else {
        return fallback;
    }
}

float Node::getFloatAttribute(std::string_view name, float fallback) const {
    if (const auto attribute = findAttribute(name)) {
        return from_string<float>(attribute.value()).value_or(fallback);
    } else {
        return fallback;
    }
}

double Node::getDoubleAttribute(std::string_view name, double fallback) const {
    if (const auto attribute = findAttribute(name)) {
        return from_string<double>(attribute.value()).value_or(fallback);
    } else {
        return fallback;
    }
}

std::string_view Node::getContent() const {
    auto node = reinterpret_cast<xmlNode*>(handle);
    auto content = xmlNodeGetContent(node);
    return content ? reinterpret_cast<const char*>(content) : std::string_view{};
}

}