#include "Image.hpp"
#include "Core/String.hpp"
#include "Core/Log.hpp"

namespace frog::preprocess {

Image::Image(PIX* pix_) : pix{ pix_ } {

}

Image::Image(std::filesystem::path path_) : path{ std::move(path_) } {
    pix = pixRead(path_to_string(path).c_str());
}

Image::~Image() {
    pixDestroy(&pix);
}

PIX* Image::getPix() const {
    return pix;
}

int Image::getWidth() const {
    return pix ? static_cast<int>(pix->w) : 0;
}

int Image::getHeight() const {
    return pix ? static_cast<int>(pix->h) : 0;
}

const std::filesystem::path& Image::getPath() const {
    return path;
}

bool Image::isOk() const {
    return pix != nullptr;
}

}
