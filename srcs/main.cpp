#include "DB.h"
#include "GUI.h"
#include "Timer.h"

void open_gui()
{
}

int main(int argc, char* argv[])
{
    if (argc == 3) {
        Timer        timer_instance(argv[1], argv[2]);
        unsigned int duration = timer_instance.run();

        DB db;
        db.save(argv[1], duration);
    } else {
        GUI gui;
        if (argc > 1)
          gui.run(argv[1]);
        else
          gui.run();
    }
    return 0;
}
