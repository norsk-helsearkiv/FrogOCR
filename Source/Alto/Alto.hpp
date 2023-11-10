#pragma once

#include "Description.hpp"
#include "Styles.hpp"
#include "Layout.hpp"

namespace frog {
class Document;
}

namespace frog::alto {

struct Alto {

    Description description;
    Styles styles;
    Layout layout;

	Alto(std::string_view xml);
	Alto(const Document& document, std::string path, int width, int height);
	Alto() = default;

};

}
