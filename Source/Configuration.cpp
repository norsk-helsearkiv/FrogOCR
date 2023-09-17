#include "Configuration.hpp"
#include "Application.hpp"

namespace frog::application {

Configuration::Configuration(xml::Document document) {
    tessdataPath = launch_path() / "tessdata";
    schemasPath = launch_path() / "Schemas";
    auto rootNode = document.getRootNode();
    for (auto node: rootNode.getChildren("")) {
        if (node.getName() == "Tessdata") {
            tessdataPath = node.getContent();
        } else if (node.getName() == "Schemas") {
            schemasPath = node.getContent();
        } else if (node.getName() == "MaxThreadCount") {
            maxThreadCount = from_string<int>(node.getContent()).value_or(0);
        } else if (node.getName() == "Database") {
            for (auto databaseNode: node.getChildren("")) {
                if (databaseNode.getName() == "Host") {
                    database.host = databaseNode.getContent();
                } else if (databaseNode.getName() == "Port") {
                    database.port = from_string<int>(databaseNode.getContent()).value_or(5432);
                } else if (databaseNode.getName() == "Name") {
                    database.name = databaseNode.getContent();
                } else if (databaseNode.getName() == "Username") {
                    database.username = databaseNode.getContent();
                } else if (databaseNode.getName() == "Password") {
                    database.password = databaseNode.getContent();
                }
            }
        } else if (node.getName() == "Python") {
            pythonPath = node.getContent();
        } else if (node.getName() == "PaddleFrog") {
            paddleFrogPath = node.getContent();
        } else if (node.getName() == "DefaultDataset") {
            defaultDataset = node.getContent();
        }
    }
}

}
