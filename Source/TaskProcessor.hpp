#pragma once

#include "OCR/Engine.hpp"
#include "Task.hpp"

#include <memory>
#include <thread>

namespace frog::application {

struct Configuration;

class TaskProcessor {
public:

    TaskProcessor(std::unique_ptr<ocr::Engine> engine);

    ~TaskProcessor();

    void pushTask(Task task);
    bool isFinished() const;
    void relaunch(const Configuration& configuration);
    int getRemainingTaskCount() const;

private:

    std::unique_ptr<ocr::Engine> engine;
    std::vector<Task> tasks;
    Task activeTask;
    std::thread thread;
    std::mutex taskMutex;

    // We start with 0 tasks and no thread, so we're technically finished after construction.
    std::atomic<bool> finished{ true };

    std::atomic<int> remainingTaskCount;

};

}
