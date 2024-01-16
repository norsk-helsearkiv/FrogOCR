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

struct HuginMuninTextRecognizerConfig {
    std::string pylaiaHtrDecodeCtcPath;
    std::string modelDirectory;
    std::string temporaryStorageDirectory;
};

struct HuginMuninTextDetectorConfig {
    std::string python;
    std::string model;
    std::string temporaryStorageDirectory;
};

struct Profile {
    std::string name;
    std::optional<TesseractConfig> tesseract;
    std::optional<PaddleTextDetectorConfig> paddleTextDetector;
    std::optional<PaddleTextAngleClassifierConfig> paddleTextOrientationClassifier;
    std::optional<HuginMuninTextRecognizerConfig> huginMuninTextRecognizer;
    std::optional<HuginMuninTextDetectorConfig> huginMuninTextDetector;
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
    int retryDatabaseConnectionIntervalSeconds{ 300 };
    int emptyTaskQueueSleepIntervalSeconds{ 30 };
    std::filesystem::path schemas;
    std::vector<DatabaseConfig> databases;
    std::vector<Profile> profiles;
    std::vector<SambaCredentialsConfig> sambaCredentials;

    Config() = default;
    Config(xml::Document document);

};

}
