#include "Activities.h"
#include "Timer.h"

#include <cstring>
#include <iostream>

static const char timer_help[] = {"activities Timer usage:\n"
                                  "\t activities time [option...]* <romFile> <processPID>\n"};

static const char global_help[] = {"activities usage:\n"
                                   "\t activities [command] [options] ...\n"
                                   "Commands:\n"
                                   "\t- gui: Display the gui\n"
                                   "\t- time: Time a game and add it to the DB.\n"
                                   "\n*use `activities [command] -h for details\n"};

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cout << global_help << std::endl;
        return 1;
    }

    if (std::strcmp(argv[1], "time") == 0) {
        if (argc == 4) {
            Timer::daemonize(argv[2], argv[3]);
        } else {
            std::cout << timer_help << std::endl;
        }
    } else if (std::strcmp(argv[1], "gui") == 0) {
        Activities& app = Activities::getInstance();
        // app runner will handle himself if argv[2] is a romfile or a flag.
        app.run(argc, argv);
    } else {
        std::cout << global_help << std::endl;
    }

    return 0;
}
