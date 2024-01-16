#pragma once

#include <leptonica/allheaders.h>
#include <opencv2/core.hpp>

#include <filesystem>

namespace frog {

struct Quad;

PIX* copy_pixels_in_quad(PIX* source, const Quad& quad);
cv::Mat pix_to_mat(PIX* pix);

}
