#include "Activities.hpp"

#include <iostream>

int main(int argc, char* argv[])
{
    // No arguments means interactive mode. Command-line flags remain supported.
    Activities& app = Activities::getInstance();
    app.run(argc, argv);
    return 0;
}
