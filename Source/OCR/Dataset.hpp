#pragma once

#include <filesystem>

namespace frog::ocr {

class Dataset {
public:

    Dataset(std::string name_, std::filesystem::path path_) : name{ std::move(name_) }, path{ std::move(path_) } {

    }

    std::string_view getName() const {
        return name;
    }

    const std::filesystem::path& getPath() const {
        return path;
    }

private:

    std::string name;
    std::filesystem::path path;

};

}
