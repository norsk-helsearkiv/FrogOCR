#pragma once

#include <leptonica/allheaders.h>
#include <opencv2/core.hpp>

#include <filesystem>

namespace frog {

struct Quad;

class Image {
public:

    Image(PIX* pix);
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

};

PIX* copy_pixels_in_quad(PIX* source, const Quad& quad);
cv::Mat pix_to_mat(PIX* pix);

}
