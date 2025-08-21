#pragma once

#include "GUI.h"

#include <SDL.h>

class GameRunner
{
  public:
    GameRunner(GUI& gui);
    ~GameRunner();

    void  stop_all();
    void  start(const std::string& romName, const std::string& system, const std::string& rom);
    pid_t wait(const std::string& romFile);

    pid_t suspend(const std::string& romFile);
    void  start_external(const std::string& command);
    void  export_childs_list();
    void  add_child(const std::string& romFile, int pid);
    pid_t get_child_pid(const std::string& romFile);

  private:
    // Track whether ra_hotkey existed when suspending a given game
    std::unordered_set<std::string> ra_hotkey_roms;
    std::map<std::string, pid_t>    childs;

    GUI& gui;

};
