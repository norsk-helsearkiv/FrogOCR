#pragma once

#include "Description.hpp"
#include "Styles.hpp"
#include "Layout.hpp"

namespace frog::preprocess {
class Image;
}

namespace frog::ocr {
class Document;
class Settings;
}

namespace frog::alto {

struct Alto {

    Description description;
    Styles styles;
    Layout layout;

	Alto(const std::filesystem::path& path);
	Alto(const ocr::Document& document, const preprocess::Image& image, const ocr::Settings& settings);
	Alto() = default;

};

}
