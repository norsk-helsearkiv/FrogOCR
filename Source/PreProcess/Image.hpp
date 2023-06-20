#pragma once

#include <leptonica/allheaders.h>

#include <filesystem>

namespace frog::preprocess {

class Image {
public:

	Image(std::filesystem::path path);
	Image(const Image&) = delete;
	Image(Image&&) = delete;

	~Image();

	Image& operator=(const Image&) = delete;
	Image& operator=(Image&&) = delete;

	PIX* getPix() const;

	int getWidth() const;
	int getHeight() const;

	const std::filesystem::path& getPath() const;

	bool isOk() const;

private:

	PIX* pix{};
    std::filesystem::path path;
    int width{};
    int height{};

};

}
