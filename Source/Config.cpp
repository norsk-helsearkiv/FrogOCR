#include "Config.hpp"
#include "Application.hpp"
#include "Core/Log.hpp"

namespace frog {

TesseractConfig load_tesseract_config_xml(xml::Node rootNode) {
    TesseractConfig config;
    for (auto node : rootNode.getChildren()) {
        if (node.getName() == "Tessdata") {
            config.tessdata = node.getContent();
        } else if (node.getName() == "Dataset") {
            config.dataset = node.getContent();
        }
    }
    if (config.tessdata.empty()) {
        config.tessdata = launch_path() / "tessdata";
        log::error("Tesseract XML configuration is missing tessdata path. Attempting fallback path: {}", config.tessdata);
    }
    if (config.dataset.empty()) {
        config.dataset = "nor";
        log::error("Tesseract XML configuration is missing dataset name. Attempting fallback dataset: {}", config.dataset);
    }
    return config;
}

PaddleTextDetectorConfig load_paddle_text_detector_config_xml(xml::Node rootNode) {
    PaddleTextDetectorConfig config;
    for (auto node : rootNode.getChildren()) {
        if (node.getName() == "Model") {
            config.model = node.getContent();
        }
    }
    if (config.model.empty()) {
        log::error("Paddle text detector XML configuration is missing model, and will not work.");
    }
    return config;
}

PaddleTextAngleClassifierConfig load_paddle_orientation_classifier_config_xml(xml::Node rootNode) {
    PaddleTextAngleClassifierConfig config;
    for (auto node : rootNode.getChildren()) {
        if (node.getName() == "Model") {
            config.model = node.getContent();
        }
    }
    if (config.model.empty()) {
        log::error("Paddle orientation classifier XML configuration is missing model, and will not work.");
    }
    return config;
}

Profile load_profile_xml(xml::Node rootNode) {
    Profile profile;
    for (auto node: rootNode.getChildren()) {
        if (node.getName() == "Tesseract") {
            profile.tesseract = load_tesseract_config_xml(node);
        } else if (node.getName() == "PaddleTextDetector") {
            profile.paddleTextDetector = load_paddle_text_detector_config_xml(node);
        } else if (node.getName() == "PaddleTextAngleClassifier") {
            profile.paddleTextOrientationClassifier = load_paddle_orientation_classifier_config_xml(node);
        }
    }
    return profile;
}

DatabaseConfig load_database_config_xml(xml::Node rootNode) {
    DatabaseConfig config;
    for (auto node: rootNode.getChildren()) {
        if (node.getName() == "Host") {
            config.host = node.getContent();
        } else if (node.getName() == "Port") {
            config.port = from_string<int>(node.getContent()).value_or(5432);
        } else if (node.getName() == "Name") {
            config.name = node.getContent();
        } else if (node.getName() == "Username") {
            config.username = node.getContent();
        } else if (node.getName() == "Password") {
            config.password = node.getContent();
        }
    }
    return config;
}

SambaCredentialsConfig load_samba_credentials_config_xml(xml::Node rootNode) {
    SambaCredentialsConfig config;
    for (auto node : rootNode.getChildren()) {
        if (node.getName() == "Username") {
            config.username = node.getContent();
        } else if (node.getName() == "Password") {
            config.password = node.getContent();
        } else if (node.getName() == "Workgroup") {
            config.workgroup = node.getContent();
        }
    }
    return config;
}

Config::Config(xml::Document document) {
    schemas = launch_path() / "Schemas";
    auto rootNode = document.getRootNode();
    for (auto node : rootNode.getChildren()) {
        if (node.getName() == "Schemas") {
            schemas = node.getContent();
        } else if (node.getName() == "MaxThreadCount") {
            maxThreadCount = from_string<int>(node.getContent()).value_or(0);
        } else if (node.getName() == "MaxTasksPerThread") {
            maxTasksPerThread = from_string<int>(node.getContent()).value_or(50);
        } else if (node.getName() == "Database") {
            databases.emplace_back(load_database_config_xml(node));
        } else if (node.getName() == "Profile") {
            profiles.emplace_back(load_profile_xml(node));
        } else if (node.getName() == "SambaCredentials") {
            sambaCredentials.emplace_back(load_samba_credentials_config_xml(node));
        }
    }
}

}
