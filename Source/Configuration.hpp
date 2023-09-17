#pragma once

#include "Core/XML/Document.hpp"

namespace frog::application {

struct Configuration {
    int maxThreadCount{};
    std::filesystem::path tessdataPath;
    std::filesystem::path schemasPath;
    std::string defaultDataset{ "nor" };
    struct {
        std::string host;
        int port{ 5432 };
        std::string name;
        std::string username;
        std::string password;
    } database;
    std::optional<std::filesystem::path> pythonPath;
    std::optional<std::filesystem::path> paddleFrogPath;

    Configuration() = default;
    Configuration(xml::Document document);
};

}
