#pragma once

#include <chrono>

namespace frog {

class Timer {
public:

	void start();
	void stop();
	void pause();
	void resume();

	bool is_paused() const;
	bool has_started() const;
	bool is_running() const;

	long long nanoseconds() const;
	long long microseconds() const;
	long long milliseconds() const;
	long long seconds() const;
	long long minutes() const;
	long long hours() const;
	long long days() const;

	double secondsAsFloat() const;

private:

	bool paused{ false };
	bool started{ false };

    std::chrono::steady_clock::time_point start_time;
    std::chrono::steady_clock::duration paused_time;

};

}
