#pragma once

#include "Description.hpp"
#include "Styles.hpp"
#include "Layout.hpp"

namespace frog {
class Image;
class Document;
}

namespace frog::alto {

struct Alto {

    Description description;
    Styles styles;
    Layout layout;

	Alto(const std::filesystem::path& path);
	Alto(const Document& document, const Image& image);
	Alto() = default;

};

}
