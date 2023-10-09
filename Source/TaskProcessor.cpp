#include "TaskProcessor.hpp"
#include "Application.hpp"
#include "Core/Log.hpp"
#include "Configuration.hpp"

namespace frog::application {

TaskProcessor::TaskProcessor(std::unique_ptr<ocr::Engine> engine_) : engine{ std::move(engine_) } {

}

TaskProcessor::~TaskProcessor() {
    if (thread.joinable()) {
        thread.join();
    }
}

void TaskProcessor::pushTask(Task task) {
    std::lock_guard lock{ taskMutex };
    tasks.emplace_back(task);
    remainingTaskCount++;
}

bool TaskProcessor::isFinished() const {
    return finished;
}

void TaskProcessor::relaunch(const Configuration& configuration) {
    if (!finished) {
        log::error("Attempted to relaunch thread before joining with current one.");
        return;
    }
    if (thread.joinable()) {
        thread.join();
    }
    finished = false;
    thread = std::thread{ [this, configuration] {
        while (!finished) {
            {
                std::lock_guard lock{ taskMutex };
                activeTask = tasks.front();
                tasks.erase(tasks.begin());
                remainingTaskCount--;
            }
            do_task(activeTask, *engine, configuration);
            if (tasks.empty()) {
                finished = true;
            }
        }
    }};
}

int TaskProcessor::getRemainingTaskCount() const {
    return remainingTaskCount;
}

}
