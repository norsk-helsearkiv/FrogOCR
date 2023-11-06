#ifdef __linux__

#include "Application.hpp"

namespace frog::gnulinux {

int argc{};
char** argv{};

}

namespace frog {

std::stack<std::string_view> launch_arguments() {
    std::stack<std::string_view> args;
    for (int i{ gnulinux::argc - 1 }; i >= 0; i--) {
        args.emplace(gnulinux::argv[i]);
    }
    return args;
}

}

int main(int argc, char** argv) {
    frog::gnulinux::argc = argc;
    frog::gnulinux::argv = argv;
    frog::start();
    return 0;
}

#endif
