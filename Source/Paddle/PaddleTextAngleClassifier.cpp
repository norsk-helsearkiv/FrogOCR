#include "PaddleTextAngleClassifier.hpp"
#include "Preprocessing.hpp"
#include "Postprocessing.hpp"
#include "Config.hpp"
#include "Core/Log.hpp"
#include "opencv2/imgproc.hpp"

namespace frog {

static void classification_resize_image(const cv::Mat& img, cv::Mat& resize_img, int width, int height) {
    const auto ratio = static_cast<float>(img.cols) / static_cast<float>(img.rows);
    const auto heightRatio = static_cast<int>(std::ceil(static_cast<float>(height) * ratio));
    const cv::Size size{ heightRatio > width ? width : heightRatio, height };
    cv::resize(img, resize_img, size, 0.0f, 0.0f, cv::INTER_LINEAR);
}

static cv::Mat get_rotated_cropped_image(const cv::Mat& srcimage, Quad quad) {
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
    log::info("Initializing Paddle text angle classifier ({})", config.model);
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

std::vector<Classification> PaddleTextAngleClassifier::classify(PIX* pix, const std::vector<Quad>& quads) {
    constexpr int tensor1{ 3 }; // I don't know what this is for.
    constexpr int width{ 192 };
    constexpr int height{ 48 };

    const auto image_matrix = pix_to_mat(pix);
    std::vector<Classification> classifications;
    classifications.reserve(quads.size());
    for (const auto& quad : quads) {
        auto quad_matrix = get_rotated_cropped_image(image_matrix, quad);
        cv::Mat resize_img;
        classification_resize_image(quad_matrix, resize_img, 192, 48);
        std::vector<float> mean{ 0.5f, 0.5f, 0.5f };
        std::vector<float> scale{ 1.0f / 0.5f, 1.0f / 0.5f, 1.0f / 0.5f };
        bool is_scale{ true };
        normalize(&resize_img, mean, scale, is_scale);
        if (resize_img.cols < width) {
            cv::copyMakeBorder(resize_img, resize_img, 0, 0, 0, width - resize_img.cols, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
        }

        std::vector<float> input;
        input.resize(tensor1 * height * width);
        permute_rgb_to_chw(resize_img, input.data());

        auto input_names = predictor->GetInputNames();
        if (input_names.empty()) {
            log::error("Input names are empty.{}");
            return {};
        }
        auto input_t = predictor->GetInputHandle(input_names[0]);
        if (!input_t) {
            log::error("Input is nullptr.");
            return {};
        }
        input_t->Reshape({ 1, tensor1, height, width });
        input_t->CopyFromCpu(input.data());
        predictor->Run();

        std::vector<float> predict_batch;
        auto output_names = predictor->GetOutputNames();
        if (output_names.empty()) {
            log::error("Output names are empty.{}");
            return {};
        }
        auto output_t = predictor->GetOutputHandle(output_names[0]);
        if (!output_t) {
            log::error("Output is nullptr.");
            return {};
        }
        auto predict_shape = output_t->shape();
        int out_num = std::accumulate(predict_shape.begin(), predict_shape.end(), 1, std::multiplies<>());
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
