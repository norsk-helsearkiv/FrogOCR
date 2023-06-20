#ifdef WIN32

#include <Windows.h>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include "Application.hpp"

namespace frog::windows {

int argc{};
char** argv{};

bool initialize_com() {
	if (const auto result = CoInitializeEx(nullptr, COINIT_MULTITHREADED); result != S_OK && result != S_FALSE) {
		warning("Failed to initialize COM library.");
		return false;
	} else {
		return true;
	}
}

void uninitialize_com() {
	CoUninitialize();
}

void initialize_console() {
	const auto stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD console_mode{ 0 };
	GetConsoleMode(stdout_handle, &console_mode);
	SetConsoleMode(stdout_handle, console_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN);
	SetConsoleOutputCP(65001);
}

}

namespace frog::application {

std::vector<std::string_view> launch_arguments() {
	std::vector<std::string_view> args;
	for (int i{ 0 }; i < windows::argc; i++) {
		args.emplace_back(windows::argv[i]);
	}
	return args;
}

}

int main(int argc, char** argv) {
    frog::windows::argc = argc;
    frog::windows::argv = argv;
	if (frog::windows::initialize_com()) {
        frog::windows::initialize_console();
        frog::application::start();
        frog::windows::uninitialize_com();
	}
	return 0;
}

#endif
