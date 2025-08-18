NAME := activities.x86
CXX := g++
CXXFLAGS := -Wall -Wextra -std=c++17 -g $(shell sdl2-config --cflags)
CXXFLAGS += -DAPP_DIR=\"./\" -DUSE_KEYBOARD
CXXFLAGS += -DVIDEO_PLAYER='"/usr/bin/ffplay "' -DMANUAL_READER='"/usr/bin/okular "'
LDLIBS := $(shell sdl2-config --libs) -lsqlite3 -lSDL2_ttf -lSDL2_image
