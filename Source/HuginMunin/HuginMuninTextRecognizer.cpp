#include "HuginMunin/HuginMuninTextRecognizer.hpp"
#include "Image.hpp"

#include <atomic>

namespace frog {

static std::atomic<int> huginMuninTextRecognizerInstanceId{ 1 };

static std::string make_hugin_munin_segmentation_config_yaml(std::string_view modelDirectory, std::string_view temporaryStorageDirectory) {
    std::string yaml;
    yaml.reserve(1024);

    yaml += "common:\n";
    yaml += fmt::format("  model_filename: {}/model\n", modelDirectory);
    yaml += fmt::format("  checkpoint: {}/weights.ckpt\n", modelDirectory);

    yaml += "decode:\n";
    yaml += "  convert_spaces: true\n";
    yaml += "  join_string: ''\n";
    yaml += "  print_line_confidence_scores: true\n";
    yaml += "  print_word_confidence_scores: true\n";
    yaml += "  segmentation: word\n";
    yaml += "  temperature: 3.0\n";
    yaml += fmt::format("  tokens_path: {}/tokens.txt\n", modelDirectory);
    yaml += fmt::format("  lexicon_path: {}/lexicon.txt\n", modelDirectory);

    yaml += "data:\n";
    yaml += "  batch_size: 1\n";

    yaml += fmt::format("syms: {}/syms.txt\n", modelDirectory);
    yaml += fmt::format("img_list: {}/list.txt\n", temporaryStorageDirectory);
    return yaml;
}

static std::string make_hugin_munin_decode_config_yaml(std::string_view modelDirectory, std::string_view temporaryStorageDirectory) {
    std::string yaml;
    yaml.reserve(1024);

    yaml += "common:\n";
    yaml += fmt::format("  model_filename: {}/model\n", modelDirectory);
    yaml += fmt::format("  checkpoint: {}/weights.ckpt\n", modelDirectory);

    yaml += "decode:\n";
    yaml += "  convert_spaces: true\n";
    yaml += "  join_string: ''\n";
    yaml += "  use_language_model: true\n";
    yaml += "  print_line_confidence_scores: true\n";
    yaml += "  temperature: 3.0\n";
    yaml += "  language_model_weight: 1.5\n";
    yaml += fmt::format("  language_model_path: {}/language_model.arpa\n", modelDirectory);
    yaml += fmt::format("  tokens_path: {}/tokens.txt\n", modelDirectory);
    yaml += fmt::format("  lexicon_path: {}/lexicon.txt\n", modelDirectory);

    yaml += "data:\n";
    yaml += "  batch_size: 1\n";
    //yaml += "  color_mode: RGB\n";

    yaml += fmt::format("syms: {}/syms.txt\n", modelDirectory);
    yaml += fmt::format("img_list: {}/list.txt\n", temporaryStorageDirectory);
    return yaml;
}

HuginMuninTextRecognizer::HuginMuninTextRecognizer(const HuginMuninTextRecognizerConfig& config_) : config{ config_ } {
    instanceId = huginMuninTextRecognizerInstanceId.fetch_add(1);
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
}


HuginMuninTextRecognizer::InputData HuginMuninTextRecognizer::createInputData(PIX* image, const std::vector<Quad>& quads) const {
    InputData inputData;
    std::size_t quadIndex{};
    for (const auto& quad : quads) {
        auto clippedPix = copy_pixels_in_quad(image, quad);
        auto rotatedPix = pixRotate(clippedPix, -quad.bottomRightToLeftAngle(), L_ROTATE_AREA_MAP, L_BRING_IN_WHITE, 0, 0);

        // Scale with aspect to 128px height to fit model.
        const auto factor = 128.0f / static_cast<float>(rotatedPix->h);
        inputData.quadScaleFactors.push_back(static_cast<float>(rotatedPix->h) / 128.0f);
        auto scaledPix = pixScale(rotatedPix, factor, factor);

        // Write to temporary storage directory.
        const auto imageFilename = fmt::format("{}/{}.jpg", instanceTemporaryStorageDirectory, quadIndex);
        pixWrite(imageFilename.c_str(), scaledPix, IFF_JFIF_JPEG);

        inputData.imagePathList += imageFilename;
        inputData.imagePathList += "\n";

        pixDestroy(&clippedPix);
        pixDestroy(&rotatedPix);
        pixDestroy(&scaledPix);
        quadIndex++;
    }
    return inputData;
}

Document HuginMuninTextRecognizer::recognize(PIX* image, const std::vector<Quad>& quads, std::vector<int> angles, const frog::TextRecognitionSettings& settings) const {
    const auto& inputData = createInputData(image, quads);
    write_file(fmt::format("{}/list.txt", instanceTemporaryStorageDirectory), inputData.imagePathList);

    const auto segmentation_config_yaml = make_hugin_munin_segmentation_config_yaml(config.modelDirectory, instanceTemporaryStorageDirectory);
    write_file(fmt::format("{}/config-segmentation.yaml", instanceTemporaryStorageDirectory), segmentation_config_yaml);

    const auto decode_config_yaml = make_hugin_munin_decode_config_yaml(config.modelDirectory, instanceTemporaryStorageDirectory);
    write_file(fmt::format("{}/config-decode.yaml", instanceTemporaryStorageDirectory), decode_config_yaml);

    // Execute PyLaia
    const auto segmentationCommand = fmt::format("{} --config \"{}/config-segmentation.yaml\"", config.pylaiaHtrDecodeCtcPath, instanceTemporaryStorageDirectory);
    const auto decodeCommand = fmt::format("{} --config \"{}/config-decode.yaml\"", config.pylaiaHtrDecodeCtcPath, instanceTemporaryStorageDirectory);
    const auto segmentationOut = executeProcessAndReturnResult(segmentationCommand);
    const auto decodeOut = executeProcessAndReturnResult(decodeCommand);
    const auto segmentationLines = split_string_view(segmentationOut, "\n");
    const auto decodeLines = split_string_view(decodeOut, "\n");
    if (segmentationLines.size() != decodeLines.size()) {
        log::error("Different number of segmentation ({}) and decode ({}) lines.", segmentationLines.size(), decodeLines.size());
        return {};
    }
    std::unordered_map<std::size_t, std::string_view> decodeLineByQuad;
    std::unordered_map<std::size_t, std::string_view> segmentationLineByQuad;
    for (const auto decodeLine : decodeLines) {
        const auto [decodeQuadIndex, decodeLineOutput] = parsePyLaiaLineForIndexAndOutput(decodeLine);
        if (decodeQuadIndex.has_value()) {
            decodeLineByQuad[decodeQuadIndex.value()] = decodeLineOutput;
        }
    }
    for (const auto segmentationLine : segmentationLines) {
        const auto [segmentationQuadIndex, segmentationLineOutput] = parsePyLaiaLineForIndexAndOutput(segmentationLine);
        if (segmentationQuadIndex.has_value()) {
            segmentationLineByQuad[segmentationQuadIndex.value()] = segmentationLineOutput;
        }
    }

    // Parse results
    Document document;
    for (std::size_t quadIndex{}; quadIndex < quads.size(); quadIndex++) {
        if (!decodeLineByQuad.contains(quadIndex) || !segmentationLineByQuad.contains(quadIndex)) {
            continue;
        }
        const auto scaleFactor = inputData.quadScaleFactors[quadIndex];
        const auto decodeLine = decodeLineByQuad[quadIndex];
        const auto segmentationLine = segmentationLineByQuad[quadIndex];
        const auto& quad = quads[quadIndex];

        const auto quadLeft = static_cast<int>(quad.left());
        const auto quadTop = static_cast<int>(quad.top());

        Line line;
        line.width = static_cast<int>(quad.width());
        line.height = static_cast<int>(quad.height());

        auto segmentationWords = parseSegmentationWords(segmentationLine);
        for (auto& word : segmentationWords) {
            word.x = static_cast<int>(static_cast<float>(word.x) * scaleFactor);
            word.y = static_cast<int>(static_cast<float>(word.y) * scaleFactor);
            word.width = static_cast<int>(static_cast<float>(word.width) * scaleFactor);
            word.height = static_cast<int>(static_cast<float>(word.height) * scaleFactor);
        }

        const auto confidence = decodeLine.substr(0, decodeLine.find(' '));
        line.confidence = { from_string<float>(confidence).value_or(0.0f), Confidence::Format::normalized };

        const auto decodeResult = decodeLine.substr(confidence.size() + 1);
        int wordCursorX{};
        std::size_t segmentedWordCursorIndex{};
        for (const auto decodeResultWord : split_string_view(decodeResult, " ")) {
            Word word;
            word.x = quadLeft + wordCursorX;
            word.y = quadTop;
            word.width = static_cast<int>(decodeResultWord.size()) * 20;
            word.height = line.height;
            word.text = decodeResultWord;
            word.confidence = line.confidence;
            bool foundSegmentedWord{};
            for (std::size_t segmentedWordIndex{ segmentedWordCursorIndex }; segmentedWordIndex < segmentationWords.size(); segmentedWordIndex++) {
                const auto& segmentationWord = segmentationWords[segmentedWordIndex];
                if (segmentationWord.text == word.text) {
                    word = segmentationWord;
                    wordCursorX += word.width;
                    foundSegmentedWord = true;
                    segmentedWordCursorIndex = segmentedWordIndex + 1;
                    break;
                }
            }
            if (!foundSegmentedWord) {
                wordCursorX += static_cast<int>(word.text.size()) * 20;
            }
            wordCursorX += 20;
            if (word.confidence.getNormalized() > 0.8f) {
                line.words.emplace_back(std::move(word));
            }
        }


        line.x = std::numeric_limits<int>::max();
        line.y = std::numeric_limits<int>::max();
        for (const auto& word : line.words) {
            line.x = std::min(line.x, word.x);
            line.y = std::min(line.y, word.y);
            line.width = std::max(line.width, word.width);
            line.height = std::max(line.height, word.height);
        }

        Paragraph paragraph;
        paragraph.x = line.x;
        paragraph.y = line.y;
        paragraph.width = line.width;
        paragraph.height = line.height;
        paragraph.lines.push_back(line);
        Block block;
        block.x = line.x;
        block.y = line.y;
        block.width = line.width;
        block.height = line.height;
        block.paragraphs.push_back(paragraph);
        document.blocks.push_back(block);
    }
    return document;
}

std::string HuginMuninTextRecognizer::executeProcessAndReturnResult(const std::string& command) const {
    std::string output;
    if (auto process = popen(reinterpret_cast<const char*>(command.c_str()), "r")) {
        constexpr std::size_t bufferSize{ 1024 * 32 };
        auto buffer = new char[bufferSize];
        while (true) {
            const auto readSize = std::fread(buffer, 1, bufferSize, process);
            if (readSize == 0) {
                break;
            }
            if (readSize < bufferSize && std::ferror(process) != 0) {
                output = {};
                break;
            }
            output.append(buffer, readSize);
        }
        pclose(process);
    }
    return output;
}

std::pair<std::optional<std::size_t>, std::string_view> HuginMuninTextRecognizer::parsePyLaiaLineForIndexAndOutput(std::string_view line) const {
    const auto endFilenameIndex = line.find(".jpg ");
    if (endFilenameIndex == std::string_view::npos || endFilenameIndex <= 1) {
        return {};
    }
    const auto endConfidenceIndex = line.find(' ', endFilenameIndex + 5);
    if (endConfidenceIndex == std::string_view::npos) {
        return {};
    }
    auto filename = line.substr(0, endFilenameIndex);
    filename = filename.substr(filename.rfind('/') + 1);
    return { from_string<std::size_t>(filename), line.substr(endFilenameIndex + 5) };
}

std::vector<Word> HuginMuninTextRecognizer::parseSegmentationWords(std::string_view segmentationLine) const {
    thread_local std::regex pattern{ R"(\(([^'\)]*('[^']*'[^'\)]*)*)\))" };
    std::vector<Word> words;
    std::cregex_iterator matchesBegin{ segmentationLine.begin(), segmentationLine.end(), pattern };
    std::cregex_iterator matchesEnd;
    for (auto match = matchesBegin; match != matchesEnd; match++) {
        const auto matchString = match->str(1);
        if (matchString.starts_with("'<space>'")) {
            continue;
        }
        const auto endTextIndex = matchString.find("', ");
        if (endTextIndex == std::string_view::npos) {
            continue;
        }
        const auto& parts = split_string_view(matchString.substr(endTextIndex + 2), ",");
        if (parts.size() != 4) {
            continue;
        }
        const auto x1 = from_string<int>(parts[0]);
        const auto y1 = from_string<int>(parts[1]);
        const auto x2 = from_string<int>(parts[2]);
        const auto y2 = from_string<int>(parts[3]);
        if (!x1.has_value() || !y1.has_value() || !x2.has_value() || !y2.has_value()) {
            continue;
        }
        Word word;
        word.x = x1.value();
        word.y = y1.value();
        word.width = x2.value() - x1.value();
        word.height = y2.value() - y1.value();
        word.text = segmentationLine.substr(0, endTextIndex);
        if (word.text != "<space>") {
            words.emplace_back(std::move(word));
        }
    }
    return words;
}

}
