#pragma once

#include "Core/XML/Document.hpp"

namespace frog {

struct SambaCredentialsConfig {
    std::string username;
    std::string password;
    std::string workgroup;
};

struct TesseractConfig {
    std::filesystem::path tessdata;
    std::string dataset;
};

struct PaddleTextDetectorConfig {
    std::filesystem::path model;
};

struct PaddleTextRecognizerConfig {
    std::filesystem::path model;
    std::filesystem::path labels;
};

struct PaddleTextAngleClassifierConfig {
    std::filesystem::path model;
};

struct Profile {
    std::string name;
    std::optional<TesseractConfig> tesseract;
    std::optional<PaddleTextDetectorConfig> paddleTextDetector;
    std::optional<PaddleTextAngleClassifierConfig> paddleTextOrientationClassifier;
};

struct DatabaseConfig {
    std::string host;
    int port{ 5432 };
    std::string name;
    std::string username;
    std::string password;
};

struct Config {

    int maxThreadCount{};
    int maxTasksPerThread{ 50 };
    std::filesystem::path schemas;
    std::vector<DatabaseConfig> databases;
    std::vector<Profile> profiles;
    std::vector<SambaCredentialsConfig> sambaCredentials;

    Config() = default;
    Config(xml::Document document);

};

}
