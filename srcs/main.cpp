#include "DB.h"
#include "GUI.h"
#include "Timer.h"

#define HELP_MESSAGE                                                                               \
    "Usage:\n\
  ./activities add <rom_file> <pid>\n\
  ./activivites del <rom_file>\n\
  ./activities gui [rom_file]"

int main(int argc, char* argv[])
{
    if (argc == 4 && std::string(argv[1]) == "add") {
        Timer        timer_instance(argv[2], argv[3]);
        unsigned int duration = timer_instance.run();

        DB db;
        db.save(argv[1], duration);
    } else if (argc == 3 && std::string(argv[1]) == "del" ) {
        DB db;
        db.remove(argv[2]);
    } else if (argc > 1 && std::string(argv[1]) == "gui") {
        GUI gui;
        gui.run(argc > 2 ? argv[2] : "");
    } else {
        std::cout << HELP_MESSAGE << std::endl;
    }

    return 0;
}
