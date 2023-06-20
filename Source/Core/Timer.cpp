#include "Timer.hpp"

namespace frog {

void Timer::start() {
	started = true;
	paused = false;
	paused_time = {};
    start_time = std::chrono::steady_clock::now();
}

void Timer::stop() {
	started = false;
	paused = false;
	paused_time = {};
	start_time = {};
}

void Timer::pause() {
	if (started && !paused) {
		paused = true;
		paused_time = std::chrono::steady_clock::now() - start_time;
	}
}

void Timer::resume() {
	paused = false;
	paused_time = {};
}

bool Timer::is_paused() const {
	return paused;
}

bool Timer::has_started() const {
	return started;
}

bool Timer::is_running() const {
	return started && !paused;
}

long long Timer::nanoseconds() const {
	return microseconds() * 1000;
}

long long Timer::microseconds() const {
	if (!started) {
		return 0;
	}
	if (paused) {
		return paused_time.count();
	}
    const auto duration = std::chrono::steady_clock::now() - start_time;
    return duration.count();
}

long long Timer::milliseconds() const {
	return microseconds() / 1000;
}

long long Timer::seconds() const {
	return microseconds() / 1000 / 1000;
}

long long Timer::minutes() const {
	return microseconds() / 1000 / 1000 / 60;
}

long long Timer::hours() const {
	return microseconds() / 1000 / 1000 / 60 / 60;
}

long long Timer::days() const {
	return microseconds() / 1000 / 1000 / 60 / 60 / 24;
}

double Timer::secondsAsFloat() const {
	return static_cast<double>(milliseconds()) / 1000.0;
}

}
