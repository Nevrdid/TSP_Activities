#include "Activities.hpp"

#include <iostream>

int main(int argc, char* argv[])
{
    Activities& app = Activities::getInstance();
    // With no arguments, start the GUI instead of exiting with usage text.
    app.run(argc, argv);
    return 0;
}
