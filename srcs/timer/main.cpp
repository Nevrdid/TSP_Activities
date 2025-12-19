#include "Timer.hpp"

#include <cstring>
#include <iostream>

static const char help[] = {"activities Timer usage:\n"
                                  "\t timer [option...]* <romFile> <processPID>\n"};

int main(int argc, char* argv[])
{
    if (argc != 3) {
        std::cout << help << std::endl;
	return 1;
    }

    Timer::daemonize(argv[1], argv[2]);

    return 0;
}
