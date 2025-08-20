#pragma once

#include "GUI.h"

#include <SDL.h>

class GameRunner
{
  public:
    GameRunner(GUI& gui);
    ~GameRunner();

    std::map<std::string, pid_t>& get_childs();
    void  stop_all();
    void  start(const std::string& romName, const std::string& system, const std::string& rom);
    pid_t wait(const std::string& romName);
    void  start_external(const std::string& command);
    void  try_rm_ra_hotkey();

  private:
    // When true, remove /tmp/trimui_inputd/ra_hotkey while GUI is displayed
    bool keep_ra_hotkey_off = false;
    // Track whether ra_hotkey existed when suspending a given game
    std::unordered_set<std::string> ra_hotkey_roms;
    std::map<std::string, pid_t>    childs;

    GUI& gui;

    pid_t get_child_pid(const std::string& romName);
};
