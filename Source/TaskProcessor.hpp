#pragma once

#include "Task.hpp"
#include "IntegratedTextDetector.hpp"
#include "Paddle/PaddleTextDetector.hpp"
#include "Tesseract/TesseractTextRecognizer.hpp"
#include "Paddle/PaddleTextAngleClassifier.hpp"
#include "HuginMunin/HuginMuninTextDetector.hpp"
#include "HuginMunin/HuginMuninTextRecognizer.hpp"
#include "Core/Log.hpp"
#include "Alto/Alto.hpp"
#include "Image.hpp"
#include "Config.hpp"
#include "Alto/WriteXml.hpp"

#include <memory>
#include <thread>
#include <vector>

namespace frog {

class TaskProcessor {
public:

    TaskProcessor(const Profile& profile);

    ~TaskProcessor();

    void pushTask(Task task);
    bool isFinished() const;
    void relaunch();
    int getRemainingTaskCount() const;

    void doTask(const Task& task);

private:

    const TextDetector* getTextDetector(std::string_view name) const;
    const TextRecognizer* getTextRecognizer(std::string_view name) const;

    std::vector<int> runTextAngleClassifier(const std::vector<Quad>& quads, const Settings& settings, PIX* pix);

    std::vector<Task> tasks;
    Task activeTask;
    std::thread thread;
    std::mutex taskMutex;

    // We start with 0 tasks and no thread, so we're technically finished after construction.
    std::atomic<bool> finished{ true };

    std::atomic<int> remainingTaskCount;

    std::unique_ptr<IntegratedTextDetector> integratedTextDetector;
    std::unique_ptr<PaddleTextDetector> paddleTextDetector;
    std::unique_ptr<TesseractTextRecognizer> tesseractTextRecognizer;
    std::unique_ptr<PaddleTextAngleClassifier> paddleTextAngleClassifier;
    std::unique_ptr<HuginMuninTextRecognizer> huginMuninTextRecognizer;
    std::unique_ptr<HuginMuninTextDetector> huginMuninTextDetector;

};

}
