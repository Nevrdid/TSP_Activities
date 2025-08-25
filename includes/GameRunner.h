#pragma once

#include "GUI.h"

#include <SDL.h>

class GameRunner
{
  private:
    GameRunner();
    GameRunner(const GameRunner& copy); 
    GameRunner& operator=(const GameRunner& copy);

    Config& cfg;
    GUI& gui;

    // Track whether ra_hotkey existed when suspending a given game
    std::unordered_set<std::string> ra_hotkey_roms;
    std::map<std::string, pid_t>    childs;

  public:
    ~GameRunner();

    static GameRunner& getInstance() {
      static GameRunner instance;
      return instance;
    }

    void stop(const std::string& romFile);
    void start(const std::string& romName, const std::string& system, const std::string& rom);
    std::pair<pid_t, int> wait(const std::string& romFile);

    pid_t suspend(const std::string& romFile);
    void  start_external(const std::string& command);
    void  export_childs_list();
    void  add_child(const std::string& romFile, int pid);
    pid_t get_child_pid(const std::string& romFile);

};
