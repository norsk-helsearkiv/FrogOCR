#include "PaddleTextDetector.hpp"
#include "opencv2/imgproc.hpp"
#include "Postprocessing.hpp"
#include "Preprocessing.hpp"

#include "Core/Log.hpp"

namespace frog {

float calculate_contour_area(const std::vector<std::vector<float>>& box, float unclip_ratio) {
    int pts_num = 4;
    float area = 0.0f;
    float dist = 0.0f;
    for (int i = 0; i < pts_num; i++) {
        area += box[i][0] * box[(i + 1) % pts_num][1] - box[i][1] * box[(i + 1) % pts_num][0];
        dist += std::sqrt((box[i][0] - box[(i + 1) % pts_num][0]) * (box[i][0] - box[(i + 1) % pts_num][0]) + (box[i][1] - box[(i + 1) % pts_num][1]) * (box[i][1] - box[(i + 1) % pts_num][1]));
    }
    area = std::abs(area / 2.0f);
    return area * unclip_ratio / dist;
}

cv::RotatedRect unclip(std::vector<std::vector<float>> box, float unclip_ratio) {
    const auto distance = calculate_contour_area(box, unclip_ratio);

    ClipperLib::ClipperOffset offset;
    std::vector<ClipperLib::IntPoint> p;
    p.emplace_back(int(box[0][0]), int(box[0][1]));
    p.emplace_back(int(box[1][0]), int(box[1][1]));
    p.emplace_back(int(box[2][0]), int(box[2][1]));
    p.emplace_back(int(box[3][0]), int(box[3][1]));
    offset.AddPath(p, ClipperLib::jtRound, ClipperLib::etClosedPolygon);

    std::vector<std::vector<ClipperLib::IntPoint>> soln;
    offset.Execute(soln, distance);
    std::vector<cv::Point2f> points;

    for (int j = 0; j < soln.size(); j++) {
        for (int i = 0; i < soln[soln.size() - 1].size(); i++) {
            points.emplace_back(soln[j][i].X, soln[j][i].Y);
        }
    }
    cv::RotatedRect res;
    if (points.empty()) {
        res = cv::RotatedRect(cv::Point2f(0, 0), cv::Size2f(1, 1), 0);
    } else {
        res = cv::minAreaRect(points);
    }
    return res;
}

Quad order_points_clockwise(Quad quad) {
    std::array<std::array<float, 2>, 4> box{
        std::array<float, 2>{ quad.x1, quad.y1 },
        std::array<float, 2>{ quad.x2, quad.y2 },
        std::array<float, 2>{ quad.x3, quad.y3 },
        std::array<float, 2>{ quad.x4, quad.y4 }
    };
    std::ranges::sort(box, [](const std::array<float, 2>& a, const std::array<float, 2>& b) {
        return a[0] < b[0];
    });
    std::array<std::array<float, 2>, 2> leftmost{ box[0], box[1] };
    std::array<std::array<float, 2>, 2> rightmost{ box[2], box[3] };
    if (leftmost[0][1] > leftmost[1][1]) {
        std::swap(leftmost[0], leftmost[1]);
    }
    if (rightmost[0][1] > rightmost[1][1]) {
        std::swap(rightmost[0], rightmost[1]);
    }
    quad.x1 = leftmost[0][0];
    quad.y1 = leftmost[0][1];
    quad.x2 = rightmost[0][0];
    quad.y2 = rightmost[0][1];
    quad.x3 = rightmost[1][0];
    quad.y3 = rightmost[1][1];
    quad.x4 = leftmost[1][0];
    quad.y4 = leftmost[1][1];
    return quad;
}

std::vector<std::vector<float>> mat_to_vector(cv::Mat mat) {
    std::vector<std::vector<float>> img_vec;
    std::vector<float> tmp;
    for (int i = 0; i < mat.rows; ++i) {
        tmp.clear();
        for (int j = 0; j < mat.cols; ++j) {
            tmp.push_back(mat.at<float>(i, j));
        }
        img_vec.push_back(tmp);
    }
    return img_vec;
}

std::vector<std::vector<float>> get_mini_boxes(cv::RotatedRect box, float& ssid) {
    ssid = std::max(box.size.width, box.size.height);

    cv::Mat points;
    cv::boxPoints(box, points);
    auto array = mat_to_vector(points);
    std::ranges::sort(array, [](const std::vector<float>& a, const std::vector<float>& b) {
        return a[0] < b[0];
    });

    std::vector<float> idx1 = array[0];
    std::vector<float> idx2 = array[1];
    std::vector<float> idx3 = array[2];
    std::vector<float> idx4 = array[3];
    if (array[3][1] <= array[2][1]) {
        idx2 = array[3];
        idx3 = array[2];
    } else {
        idx2 = array[2];
        idx3 = array[3];
    }
    if (array[1][1] <= array[0][1]) {
        idx1 = array[1];
        idx4 = array[0];
    } else {
        idx1 = array[0];
        idx4 = array[1];
    }

    array[0] = idx1;
    array[1] = idx2;
    array[2] = idx3;
    array[3] = idx4;
    return array;
}

float accumulated_polygon_score(std::vector<cv::Point> contour, cv::Mat pred) {
    int width = pred.cols;
    int height = pred.rows;
    std::vector<float> box_x;
    std::vector<float> box_y;
    for (auto& point : contour) {
        box_x.push_back(static_cast<float>(point.x));
        box_y.push_back(static_cast<float>(point.y));
    }
    const auto xmin = std::clamp(static_cast<int>(std::floor(*(std::ranges::min_element(box_x)))), 0, width - 1);
    const auto xmax = std::clamp(static_cast<int>(std::ceil(*(std::ranges::max_element(box_x)))), 0, width - 1);
    const auto ymin = std::clamp(static_cast<int>(std::floor(*(std::ranges::min_element(box_y)))), 0, height - 1);
    const auto ymax = std::clamp(static_cast<int>(std::ceil(*(std::ranges::max_element(box_y)))), 0, height - 1);

    cv::Mat mask;
    mask = cv::Mat::zeros(ymax - ymin + 1, xmax - xmin + 1, CV_8UC1);

    std::vector<cv::Point> rook_points;
    rook_points.reserve(contour.size());
    for (int i = 0; i < contour.size(); i++) {
        rook_points.emplace_back(static_cast<int>(box_x[i]) - xmin, static_cast<int>(box_y[i]) - ymin);
    }
    const cv::Point* ppt[]{ rook_points.data() };
    int npt[]{ static_cast<int>(contour.size()) };
    cv::fillPoly(mask, ppt, npt, 1, cv::Scalar(1));
    cv::Mat croppedImg;
    pred(cv::Rect(xmin, ymin, xmax - xmin + 1, ymax - ymin + 1)).copyTo(croppedImg);
    auto score = cv::mean(croppedImg, mask)[0];
    return static_cast<float>(score);
}

std::vector<Quad> quads_from_bitmap(const cv::Mat& pred, const cv::Mat& bitmap, const float& box_thresh, const float& det_db_unclip_ratio) {
    const int min_size{ 3 };
    const std::size_t max_candidates{ 1000 };
    const auto width = static_cast<float>(bitmap.cols);
    const auto height = static_cast<float>(bitmap.rows);
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(bitmap, contours, hierarchy, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);
    const auto num_contours = contours.size() >= max_candidates ? max_candidates : contours.size();
    std::vector<Quad> boxes;
    for (int contour_index = 0; contour_index < num_contours; contour_index++) {
        if (contours[contour_index].size() <= 2) {
            continue;
        }
        float ssid{};
        auto array = get_mini_boxes(cv::minAreaRect(contours[contour_index]), ssid);
        if (ssid < min_size) {
            continue;
        }
        auto score = accumulated_polygon_score(contours[contour_index], pred);
        if (score < box_thresh) {
            continue;
        }
        auto points = unclip(array, det_db_unclip_ratio);
        if (points.size.height < 1.001 && points.size.width < 1.001) {
            continue;
        }
        auto clip_array = get_mini_boxes(points, ssid);
        if (ssid < min_size + 2) {
            continue;
        }
        const auto dest_width = static_cast<float>(pred.cols);
        const auto dest_height = static_cast<float>(pred.rows);
        Quad quad;
        quad.x1 = std::clamp(std::round(clip_array[0][0] / width * dest_width), 0.0f, dest_width);
        quad.y1 = std::clamp(std::round(clip_array[0][1] / height * dest_height), 0.0f, dest_height);
        quad.x2 = std::clamp(std::round(clip_array[1][0] / width * dest_width), 0.0f, dest_width);
        quad.y2 = std::clamp(std::round(clip_array[1][1] / height * dest_height), 0.0f, dest_height);
        quad.x3 = std::clamp(std::round(clip_array[2][0] / width * dest_width), 0.0f, dest_width);
        quad.y3 = std::clamp(std::round(clip_array[2][1] / height * dest_height), 0.0f, dest_height);
        quad.x4 = std::clamp(std::round(clip_array[3][0] / width * dest_width), 0.0f, dest_width);
        quad.y4 = std::clamp(std::round(clip_array[3][1] / height * dest_height), 0.0f, dest_height);
        boxes.push_back(quad);
    }
    return boxes;
}

std::vector<Quad> filter_tag_det_res(std::vector<Quad> boxes, float ratio_h, float ratio_w, cv::Mat srcimg) {
    const auto original_image_h = static_cast<float>(srcimg.rows);
    const auto original_image_w = static_cast<float>(srcimg.cols);
    std::vector<Quad> root_points;
    for (auto& box : boxes) {
        box = order_points_clockwise(box);

        box.x1 /= ratio_w;
        box.y1 /= ratio_h;
        box.x1 = std::min(std::max(box.x1, 0.0f), original_image_w - 1.0f);
        box.y1 = std::min(std::max(box.y1, 0.0f), original_image_h - 1.0f);

        box.x2 /= ratio_w;
        box.y2 /= ratio_h;
        box.x2 = std::min(std::max(box.x2, 0.0f), original_image_w - 1.0f);
        box.y2 = std::min(std::max(box.y2, 0.0f), original_image_h - 1.0f);

        box.x3 /= ratio_w;
        box.y3 /= ratio_h;
        box.x3 = std::min(std::max(box.x3, 0.0f), original_image_w - 1.0f);
        box.y3 = std::min(std::max(box.y3, 0.0f), original_image_h - 1.0f);

        box.x4 /= ratio_w;
        box.y4 /= ratio_h;
        box.x4 = std::min(std::max(box.x4, 0.0f), original_image_w - 1.0f);
        box.y4 = std::min(std::max(box.y4, 0.0f), original_image_h - 1.0f);

        if (std::sqrt(std::pow(box.x1 - box.x2, 2) + std::pow(box.y1 - box.y2, 2)) > 4.0f) {
            if (std::sqrt(std::pow(box.x1 - box.x4, 2) + std::pow(box.y1 - box.y4, 2)) > 4.0f) {
                root_points.push_back(box);
            }
        }
    }
    return root_points;
}

PaddleTextDetector::PaddleTextDetector(const PaddleTextDetectorConfig& config) {
    log::info("Initializing Paddle text detector ({})", config.model);
    if (!std::filesystem::exists(config.model)) {
        log::error("Unable to find detection model at configured path: {}", config.model);
        return;
    }
    const auto model_directory = path_to_string(config.model);
    paddle_infer::Config paddleConfig;
    paddleConfig.SetModel(model_directory + "/inference.pdmodel", model_directory + "/inference.pdiparams");
    paddleConfig.DisableGpu();
    paddleConfig.EnableMKLDNN();
    paddleConfig.SetMkldnnCacheCapacity(10);
    paddleConfig.SetCpuMathLibraryNumThreads(1);
    paddleConfig.SwitchUseFeedFetchOps(false);
    paddleConfig.SwitchSpecifyInputNames(true);
    paddleConfig.SwitchIrOptim(true);
    paddleConfig.EnableMemoryOptim();
#ifndef FROG_DEBUG_LOG_PADDLE
    paddleConfig.DisableGlogInfo();
#endif
    predictor = paddle_infer::CreatePredictor(paddleConfig);
}

std::vector<Quad> PaddleTextDetector::detect(const Image& image, const TextDetectionSettings& settings) {
    const auto image_matrix = pix_to_mat(image.getPix());

    float ratio_h{};
    float ratio_w{};

    cv::Mat srcimg;
    cv::Mat resize_img;
    image_matrix.copyTo(srcimg);

    int limit_side_length{ 960 };
    resize_image(image_matrix, resize_img, limit_side_length, ratio_h, ratio_w);
    std::vector<float> mean{ 0.485f, 0.456f, 0.406f };
    std::vector<float> scale{ 1.0f / 0.229f, 1.0f / 0.224f, 1.0f / 0.225f };
    bool is_scale{ true };
    normalize(&resize_img, mean, scale, is_scale);
    std::vector<float> input(3 * resize_img.rows * resize_img.cols, 0.0f);
    permute_rgb_to_chw(resize_img, input.data());

    auto input_names = predictor->GetInputNames();
    auto input_t = predictor->GetInputHandle(input_names[0]);
    if (!input_t) {
        log::error("Input is nullptr.");
        return {};
    }
    input_t->Reshape({ 1, 3, resize_img.rows, resize_img.cols });
    input_t->CopyFromCpu(input.data());
    predictor->Run();

    std::vector<float> out_data;
    auto output_names = predictor->GetOutputNames();
    auto output_t = predictor->GetOutputHandle(output_names[0]);
    if (!output_t) {
        log::error("Output is nullptr.");
        return {};
    }
    auto output_shape = output_t->shape();
    int out_num = std::accumulate(output_shape.begin(), output_shape.end(), 1, std::multiplies<int>());

    out_data.resize(out_num);
    output_t->CopyToCpu(out_data.data());

    int n2 = output_shape[2];
    int n3 = output_shape[3];
    int n = n2 * n3;

    std::vector<float> pred(n, 0.0f);
    std::vector<unsigned char> cbuf(n, ' ');

    for (int i = 0; i < n; i++) {
        pred[i] = out_data[i];
        cbuf[i] = static_cast<unsigned char>(out_data[i] * 255.0f);
    }

    cv::Mat cbuf_map(n2, n3, CV_8UC1, cbuf.data());
    cv::Mat pred_map(n2, n3, CV_32F, pred.data());

    const double det_db_threshold{ 0.3 };
    const auto threshold = det_db_threshold * 255.0;
    const double maxvalue{ 255.0 };
    cv::Mat bit_map;
    cv::threshold(cbuf_map, bit_map, threshold, maxvalue, cv::THRESH_BINARY);
    if (settings.find("Paddle.UseDilation") == "true") {
        auto dila_ele = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2, 2));
        cv::dilate(bit_map, bit_map, dila_ele);
    }
    float det_db_box_threshold{ 0.6f };
    float det_db_unclip_ratio{ 1.5f };
    auto quads = quads_from_bitmap(pred_map, bit_map, det_db_box_threshold, det_db_unclip_ratio);
    quads = filter_tag_det_res(quads, ratio_h, ratio_w, srcimg);

    std::ranges::sort(quads, [](const Quad& a, const Quad& b) {
        if (a.y1 < b.y1) {
            return true;
        } else if (a.y1 == b.y1) {
            return a.x1 < b.x1;
        } else {
            return false;
        }
    });
    for (int i{ 0 }; i < static_cast<int>(quads.size()) - 1; i++) {
        for (int j = i; j >= 0; j--) {
            if (std::abs(quads[j + 1].y1 - quads[j].y1) < 10 && (quads[j + 1].x1 < quads[j].x1)) {
                std::swap(quads[i], quads[i + 1]);
            }
        }
    }
    return quads;
}

}
