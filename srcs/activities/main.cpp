#include "Activities.hpp"

#include <cstring>
#include <iostream>

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cout << help << std::endl;
        return 1;
    }

    Activities& app = Activities::getInstance();
    // app runner will handle himself if argv[2] is a romfile or a flag.
    app.run(argc, argv);

    return 0;
}
