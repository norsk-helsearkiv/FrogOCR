#include "Install.hpp"
#include "Core/Log.hpp"
#include "Config.hpp"

namespace frog {

constexpr std::string_view defaultConfiguration{
    R"(<?xml version="1.0" encoding="UTF-8"?>)"
    "\n"
    "<Configuration>\n"
    "\t<MaxThreadCount>0</MaxThreadCount>\n"
    "\t<MaxTasksPerThread>50</MaxTasksPerThread>\n"
    "\t<Schemas>/etc/frog/schemas</Schemas>\n"
    "\t<RetryDatabaseConnectionIntervalSeconds>300</RetryDatabaseConnectionIntervalSeconds>\n"
    "\t<EmptyTaskQueueSleepIntervalSeconds>30</EmptyTaskQueueSleepIntervalSeconds>\n"

    "\t<Database role=\"all\">\n"
    "\t\t<Host>localhost</Host>\n"
    "\t\t<Port>5432</Port>\n"
    "\t\t<Name>frog</Name>\n"
    "\t\t<Username>frog</Username>\n"
    "\t\t<Password>frog</Password>\n"
    "\t</Database>\n"

    "\t<!--<SambaCredentials>\n"
    "\t\t<Username></Username>\n"
    "\t\t<Password></Password>\n"
    "\t</SambaCredentials>-->\n"

    "\t<Profile>\n"
    "\t\t<Tesseract>\n"
    "\t\t\t<Tessdata>/etc/frog/tessdata</Tessdata>\n"
    "\t\t\t<Dataset>nor</Dataset>\n"
    "\t\t</Tesseract>\n"

    "\t\t<PaddleTextDetector>\n"
    "\t\t\t<Model>/etc/frog/paddle/en_PP-OCRv3_det_infer</Model>\n"
    "\t\t</PaddleTextDetector>\n"

    "\t\t<!--<PaddleTextAngleClassifier>\n"
    "\t\t\t<Model>/etc/frog/paddle/ch_ppocr_mobile_v2.0_cls_infer</Model>\n"
    "\t\t</PaddleTextAngleClassifier>-->\n"
    "\t</Profile>\n"

    "</Configuration>\n"
};

void cli_config(std::stack<std::string_view> arguments) {
    while (!arguments.empty()) {
        const auto argument = arguments.top();
        arguments.pop();
        if (argument == "--print-default") {
            fmt::print("{}", defaultConfiguration);
            return;
        }
    }
    std::error_code errorCode;
    if (!std::filesystem::is_directory("/etc/frog")) {
        if (!std::filesystem::create_directory("/etc/frog", errorCode)) {
            log::error("Failed to create directory: %cyan/etc/frog");
            return;
        }
    }
    if (std::filesystem::exists("/etc/frog/config.xml")) {
        log::error("File already exists: %cyan/etc/frog/config.xml");
        return;
    }
    if (!write_file("/etc/frog/config.xml", defaultConfiguration)) {
        log::error("Failed to create file %cyan/etc/frog/config.xml");
    }
}

}
