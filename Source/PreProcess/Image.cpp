#include "Image.hpp"
#include "Core/String.hpp"
#include "Core/Log.hpp"

namespace frog::preprocess {

Image::Image(std::filesystem::path path_) : path{ std::move(path_) } {
    pix = pixRead(path_to_string(path).c_str());
    if (!pix) {
        return;
    }
    width = static_cast<int>(pix->w);
    height = static_cast<int>(pix->h);
}

Image::~Image() {
    pixDestroy(&pix);
}

PIX* Image::getPix() const {
    return pix;
}

int Image::getWidth() const {
    return width;
}

int Image::getHeight() const {
    return height;
}

const std::filesystem::path& Image::getPath() const {
    return path;
}

bool Image::isOk() const {
    return pix != nullptr;
}

}
