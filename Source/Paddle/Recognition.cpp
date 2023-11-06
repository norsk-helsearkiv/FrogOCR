#include "Recognition.hpp"
#include "Postprocessing.hpp"
#include "Preprocessing.hpp"

namespace frog {

PaddleRecognizer::PaddleRecognizer(std::string model_directory, std::string label_path) {
    labels = Utility::ReadDict(label_path);
    labels.insert(labels.begin(), "#"); // blank char for ctc
    labels.emplace_back(" ");
    LoadModel(model_directory);
}

void PaddleRecognizer::LoadModel(std::string model_directory) {
    paddle_infer::Config config;
    config.SetModel(model_directory + "/inference.pdmodel", model_directory + "/inference.pdiparams");
    config.DisableGpu();
    config.EnableMKLDNN();
    config.SetMkldnnCacheCapacity(10);
    config.SetCpuMathLibraryNumThreads(1);
    auto pass_builder = config.pass_builder();
    pass_builder->DeletePass("matmul_transpose_reshape_fuse_pass");
    config.SwitchUseFeedFetchOps(false);
    config.SwitchSpecifyInputNames(true);
    config.SwitchIrOptim(true);
    config.EnableMemoryOptim();
#ifndef FROG_DEBUG_LOG_PADDLE
    config.DisableGlogInfo();
#endif
    predictor = paddle_infer::CreatePredictor(config);
}

void PaddleRecognizer::Run(std::vector<cv::Mat> img_list, std::vector<std::string>& rec_texts, std::vector<float>& rec_text_scores) {
    int img_num = img_list.size();
    std::vector<float> width_list;
    for (int i = 0; i < img_num; i++) {
        width_list.push_back(float(img_list[i].cols) / img_list[i].rows);
    }
    std::vector<int> indices;
    indices.resize(width_list.size());
    for (std::size_t i{ 0 }; i < width_list.size(); i++) {
        indices.emplace_back(static_cast<int>(i));
    }
    std::ranges::sort(indices, [&width_list](int a, int b) {
        return width_list[a] < width_list[b];
    });

    std::vector<float> mean{ 0.5f, 0.5f, 0.5f };
    std::vector<float> scale{ 1 / 0.5f, 1 / 0.5f, 1 / 0.5f };
    bool is_scale{ true };

    int rec_img_height{ 48 };
    int rec_img_width{ 320};

    std::vector<int> rec_image_shape{ 3, rec_img_height, rec_img_width };
    int rec_batch_num_{ 1 };

    for (int beg_img_no = 0; beg_img_no < img_num; beg_img_no += rec_batch_num_) {
        int end_img_no = std::min(img_num, beg_img_no + rec_batch_num_);
        int batch_num = end_img_no - beg_img_no;
        int imgH = rec_image_shape[1];
        int imgW = rec_image_shape[2];
        float max_wh_ratio = imgW * 1.0 / imgH;
        for (int ino = beg_img_no; ino < end_img_no; ino++) {
            int h = img_list[indices[ino]].rows;
            int w = img_list[indices[ino]].cols;
            float wh_ratio = w * 1.0 / h;
            max_wh_ratio = std::max(max_wh_ratio, wh_ratio);
        }

        int batch_width = imgW;
        std::vector<cv::Mat> norm_img_batch;
        for (int ino = beg_img_no; ino < end_img_no; ino++) {
            cv::Mat srcimg;
            img_list[indices[ino]].copyTo(srcimg);
            cv::Mat resize_img;
            recognition_resize_image(srcimg, resize_img, max_wh_ratio, rec_image_shape);
            normalize(&resize_img, mean, scale, is_scale);
            norm_img_batch.push_back(resize_img);
            batch_width = std::max(resize_img.cols, batch_width);
        }

        std::vector<float> input(batch_num * 3 * imgH * batch_width, 0.0f);
        permute_rgb_to_chw_batch(norm_img_batch, input.data());
        // Inference.
        auto input_names = predictor->GetInputNames();
        auto input_t = predictor->GetInputHandle(input_names[0]);
        input_t->Reshape({ batch_num, 3, imgH, batch_width });
        input_t->CopyFromCpu(input.data());
        predictor->Run();

        std::vector<float> predict_batch;
        auto output_names = predictor->GetOutputNames();
        auto output_t = predictor->GetOutputHandle(output_names[0]);
        auto predict_shape = output_t->shape();

        int out_num = std::accumulate(predict_shape.begin(), predict_shape.end(), 1, std::multiplies<int>());
        predict_batch.resize(out_num);
        // predict_batch is the result of Last FC with softmax
        output_t->CopyToCpu(predict_batch.data());
        // ctc decode
        for (int m = 0; m < predict_shape[0]; m++) {
            std::string str_res;
            int argmax_idx{};
            int last_index{};
            float score{};
            int count{};
            float max_value{};
            for (int n = 0; n < predict_shape[1]; n++) {
                // get idx
                argmax_idx = int(Utility::argmax(&predict_batch[(m * predict_shape[1] + n) * predict_shape[2]], &predict_batch[(m * predict_shape[1] + n + 1) * predict_shape[2]]));
                // get score
                max_value = float(*std::max_element(&predict_batch[(m * predict_shape[1] + n) * predict_shape[2]], &predict_batch[(m * predict_shape[1] + n + 1) * predict_shape[2]]));

                if (argmax_idx > 0 && (!(n > 0 && argmax_idx == last_index))) {
                    score += max_value;
                    count++;
                    str_res += labels[argmax_idx];
                }
                last_index = argmax_idx;
            }
            score /= static_cast<float>(count);
            if (std::isnan(score)) {
                continue;
            }
            rec_texts[indices[beg_img_no + m]] = str_res;
            rec_text_scores[indices[beg_img_no + m]] = score;
        }
    }
}

}
