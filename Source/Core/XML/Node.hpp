#pragma once

#include <optional>
#include <string>
#include <vector>
#include <filesystem>
#include <functional>

namespace frog::xml {

class Node {
public:

    friend class Document;

    Node(const Node&) = default;
    Node(Node&&) = default;

    ~Node() = default;

    Node& operator=(const Node&) = default;
    Node& operator=(Node&&) = default;

    std::string_view getName() const;

    std::vector<Node> getChildren(std::string_view tagName = {}) const;
    std::optional<Node> findFirstChild(std::string_view tagName = {}) const;

    std::optional<std::string> findAttribute(std::string_view name) const;

    std::string getAttribute(std::string_view name, std::string fallback = "") const;
    int getIntAttribute(std::string_view name, int fallback = 0) const;
    float getFloatAttribute(std::string_view name, float fallback = 0.0f) const;
    double getDoubleAttribute(std::string_view name, double fallback = 0.0) const;

    std::string_view getContent() const;

private:

    Node(void* handle);

    void* handle{};

};

}
