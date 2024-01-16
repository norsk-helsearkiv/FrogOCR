#include "HuginMunin/HuginMuninTextDetector.hpp"

#include <atomic>

namespace frog {

// classes:
//   - background
//   - text_line_horizontal
//   - text_line_vertical
constexpr std::string_view line_detection_py{ R"(import sys
import cv2
import logging
from doc_ufcn.main import DocUFCN
if __name__ == '__main__':
    image_path = sys.argv[1]
    model_path = sys.argv[2]
    logging.basicConfig(format="[%(levelname)s] %(message)s", stream=sys.stdout, level=logging.INFO)
    image = cv2.cvtColor(cv2.imread(image_path), cv2.COLOR_BGR2RGB)
    nb_of_classes = 3
    mean = [209, 204, 191]
    std = [51, 51, 50]
    input_size = 768
    model = DocUFCN(nb_of_classes, input_size, 'cpu')
    model.load(model_path, mean, std, mode="eval")
    classes_with_polygons = model.predict(image, min_cc=50)
    for class_with_polygons in classes_with_polygons:
        if class_with_polygons is None:
            continue
        for class_index in class_with_polygons:
            print('class', class_index)
            for polygon in class_with_polygons[class_index]:
                confidence = polygon['confidence']
                points = polygon['polygon']
                print('polygon', confidence)
                for point in points:
                    print(int(point[0]), int(point[1]), end=' ')
                print()
                print('end-polygon')
)"
};

static std::atomic<int> huginMuninTextDetectorInstanceId{ 1 };

HuginMuninTextDetector::HuginMuninTextDetector(const HuginMuninTextDetectorConfig& config_) : config{ config_ } {
    instanceId = huginMuninTextDetectorInstanceId.fetch_add(1);
    instanceTemporaryStorageDirectory = fmt::format("{}/{}", config.temporaryStorageDirectory, instanceId);

    // Ensure the temporary storage for this instance exists and is empty.
    std::error_code errorCode;
    if (std::filesystem::exists(instanceTemporaryStorageDirectory, errorCode)) {
        for (const auto& entry : std::filesystem::directory_iterator(instanceTemporaryStorageDirectory, errorCode)) {
            std::filesystem::remove_all(entry.path(), errorCode);
        }
        std::filesystem::create_directory(instanceTemporaryStorageDirectory, errorCode);
    } else {
        std::filesystem::create_directories(config.temporaryStorageDirectory, errorCode);
    }

    pythonScriptPath = fmt::format("{}/line-detection", instanceTemporaryStorageDirectory);
    write_file(pythonScriptPath, fmt::format("#!{}\n{}", config.python, line_detection_py));
    std::filesystem::permissions(pythonScriptPath, std::filesystem::perms::all, std::filesystem::perm_options::add, errorCode);
}

std::vector<Quad> HuginMuninTextDetector::detect(PIX* image, const TextDetectionSettings& settings) const {
    const auto imageTempFilename = fmt::format("{}/full.jpg", instanceTemporaryStorageDirectory);
    pixWrite(imageTempFilename.c_str(), image, IFF_JFIF_JPEG);

    // Run Doc-UFCN line detection
    std::string docUfcnOut;
    const auto commandTemp = fmt::format("{} {} {}", pythonScriptPath, imageTempFilename, config.model);
    if (auto process = popen(reinterpret_cast<const char*>(commandTemp.c_str()), "r")) {
        constexpr std::size_t bufferSize{ 1024 * 32 };
        auto buffer = new char[bufferSize];
        while (true) {
            const auto readSize = std::fread(buffer, 1, bufferSize, process);
            if (readSize == 0) {
                break;
            }
            if (readSize < bufferSize && std::ferror(process) != 0) {
                docUfcnOut = {};
                break;
            }
            docUfcnOut.append(buffer, readSize);
        }
        pclose(process);
    }

    // Parse line results
    std::vector<Quad> quads;
    int currentClass{};
    Quad quad;
    for (const auto& docUfcnOutLine : split_string_view(docUfcnOut, "\n")) {
        if (docUfcnOutLine.starts_with("class ")) {
            currentClass = from_string<int>(docUfcnOutLine.substr(6)).value_or(0);
            continue;
        }
        if (currentClass == 0) {
            continue;
        }
        if (docUfcnOutLine.starts_with("polygon ")) {
            // confidence = from_string<float>(docUfcnOutLine.substr(8));
            quad = {};
            quad.x1 = std::numeric_limits<float>::max();
            quad.x2 = std::numeric_limits<float>::min();
            quad.x3 = std::numeric_limits<float>::min();
            quad.x4 = std::numeric_limits<float>::max();
            quad.y1 = std::numeric_limits<float>::max();
            quad.y2 = std::numeric_limits<float>::max();
            quad.y3 = std::numeric_limits<float>::min();
            quad.y4 = std::numeric_limits<float>::min();
            continue;
        }
        if (docUfcnOutLine.starts_with("end-polygon")) {
            quads.push_back(quad);
            continue;
        }
        char component{ 'x' };
        for (const auto pointComponent : split_string_view(docUfcnOutLine, " ")) {
            if (pointComponent.empty()) {
                continue;
            }
            if (component == 'x') {
                if (const auto x = from_string<float>(pointComponent)) {
                    quad.x1 = std::min(quad.x1, x.value());
                    quad.x2 = std::max(quad.x2, x.value());
                    quad.x3 = std::max(quad.x3, x.value());
                    quad.x4 = std::min(quad.x4, x.value());
                    component = 'y';
                }
            } else {
                if (const auto y = from_string<float>(pointComponent)) {
                    quad.y1 = std::min(quad.y1, y.value());
                    quad.y2 = std::min(quad.y2, y.value());
                    quad.y3 = std::max(quad.y3, y.value());
                    quad.y4 = std::max(quad.y4, y.value());
                    component = 'x';
                }
            }
        }
    }
    return quads;
}

}
