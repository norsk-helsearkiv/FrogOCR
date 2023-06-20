#pragma once

#include "Core/XML/Document.hpp"

namespace frog::application {

struct Configuration {
    int maxThreadCount{};
    std::filesystem::path tessdataPath;
    std::filesystem::path schemasPath;
    struct {
        std::string host;
        int port{ 5432 };
        std::string name;
        std::string username;
        std::string password;
    } database;

    Configuration() = default;
    Configuration(xml::Document document);
};

}
