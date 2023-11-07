#include "PaddleTextAngleClassifier.hpp"
#include "Preprocessing.hpp"
#include "Postprocessing.hpp"
#include "Config.hpp"
#include "Core/Log.hpp"
#include "Image.hpp"
#include "opencv2/imgproc.hpp"

namespace frog {

cv::Mat get_rotated_cropped_image(const cv::Mat& srcimage, Quad quad) {
    const auto left = quad.left();
    const auto top = quad.top();

    cv::Mat image;
    srcimage.copyTo(image);

    cv::Mat img_crop;
    image(cv::Rect2f(left, top, quad.right() - left, quad.bottom() - top)).copyTo(img_crop);
    quad.x1 -= left;
    quad.y1 -= top;
    quad.x2 -= left;
    quad.y2 -= top;
    quad.x3 -= left;
    quad.y3 -= top;
    quad.x4 -= left;
    quad.y4 -= top;

    const auto cropped_width = std::sqrt(std::pow(quad.x1 - quad.x2, 2.0f) + std::pow(quad.y1 - quad.y2, 2.0f));
    const auto cropped_height = std::sqrt(std::pow(quad.x1 - quad.x4, 2.0f) + std::pow(quad.y1 - quad.y4, 2.0f));

    const cv::Point2f source[4]{
        { 0.0f,          0.0f },
        { cropped_width, 0.0f },
        { cropped_width, cropped_height },
        { 0.0f,          cropped_height }
    };

    const cv::Point2f destination[4]{
        { quad.x1, quad.y1 },
        { quad.x2, quad.y2 },
        { quad.x3, quad.y3 },
        { quad.x4, quad.y4 }
    };

    cv::Mat M = cv::getPerspectiveTransform(source, destination);
    cv::Mat dst_img;
    cv::warpPerspective(img_crop, dst_img, M, cv::Size(cropped_width, cropped_height), cv::BORDER_REPLICATE);

    if (static_cast<float>(dst_img.rows) >= static_cast<float>(dst_img.cols) * 1.5f) {
        cv::Mat srcCopy = cv::Mat(dst_img.rows, dst_img.cols, dst_img.depth());
        cv::transpose(dst_img, srcCopy);
        cv::flip(srcCopy, srcCopy, 0);
        return srcCopy;
    } else {
        return dst_img;
    }
}

PaddleTextAngleClassifier::PaddleTextAngleClassifier(const PaddleTextAngleClassifierConfig& config) {
    if (!std::filesystem::exists(config.model)) {
        log::error("Unable to find detection model at configured path: {}", config.model);
        return;
    }
    fmt::print("Initializing Paddle classifier ({})\n", config.model);
    const auto model_directory = path_to_string(config.model);
    paddle_infer::Config paddleConfig;
    paddleConfig.SetModel(model_directory + "/inference.pdmodel", model_directory + "/inference.pdiparams");
    paddleConfig.DisableGpu();
    paddleConfig.EnableMKLDNN();
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

std::vector<Classification> PaddleTextAngleClassifier::classify(const Image& image, const std::vector<Quad>& quads) {
    std::vector<int> cls_image_shape{ 3, 48, 192 };
    const auto image_matrix = pix_to_mat(image.getPix());
    std::vector<Classification> classifications;
    classifications.reserve(quads.size());
    for (const auto& quad : quads) {
        auto quad_matrix = get_rotated_cropped_image(image_matrix, quad);
        cv::Mat resize_img;
        classification_resize_image(quad_matrix, resize_img, cls_image_shape);
        std::vector<float> mean{ 0.5f, 0.5f, 0.5f };
        std::vector<float> scale{ 1.0f / 0.5f, 1.0f / 0.5f, 1.0f / 0.5f };
        bool is_scale{ true };
        normalize(&resize_img, mean, scale, is_scale);
        if (resize_img.cols < cls_image_shape[2]) {
            cv::copyMakeBorder(resize_img, resize_img, 0, 0, 0, cls_image_shape[2] - resize_img.cols, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
        }

        std::vector<float> input(cls_image_shape[0] * cls_image_shape[1] * cls_image_shape[2], 0.0f);
        permute_rgb_to_chw(resize_img, input.data());

        auto input_names = predictor->GetInputNames();
        auto input_t = predictor->GetInputHandle(input_names[0]);
        input_t->Reshape({ 1, cls_image_shape[0], cls_image_shape[1], cls_image_shape[2] });
        input_t->CopyFromCpu(input.data());
        predictor->Run();

        std::vector<float> predict_batch;
        auto output_names = predictor->GetOutputNames();
        auto output_t = predictor->GetOutputHandle(output_names[0]);
        auto predict_shape = output_t->shape();
        int out_num = std::accumulate(predict_shape.begin(), predict_shape.end(), 1, std::multiplies<int>());
        predict_batch.resize(out_num);
        output_t->CopyToCpu(predict_batch.data());

        Classification classification;
        classification.label = std::distance(&predict_batch[0], std::max_element(&predict_batch[0], &predict_batch[predict_shape[1]]));
        classification.confidence = *std::max_element(&predict_batch[0], &predict_batch[predict_shape[1]]);
        classifications.push_back(classification);
    }
    return classifications;
}

}
