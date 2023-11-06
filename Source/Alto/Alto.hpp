#pragma once

#include "Description.hpp"
#include "Styles.hpp"
#include "Layout.hpp"

namespace frog {
class Image;
class Document;
class Settings;
}

namespace frog::alto {

struct Alto {

    Description description;
    Styles styles;
    Layout layout;

	Alto(const std::filesystem::path& path);
	Alto(const Document& document, const Image& image, const Settings& settings);
	Alto() = default;

};

}
