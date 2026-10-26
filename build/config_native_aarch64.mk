# Native ARM64 build inside the Debian Bullseye ARM64 build container.
# Keep this environment pinned to Bullseye for handheld runtime compatibility.
EXT :=
CXX := g++
CXXFLAGS := -Wall -Wextra -std=c++17 $(shell sdl2-config --cflags)
CXXFLAGS += -DAPP_DIR='"/mnt/SDCARD/Apps/Activities/"' \
            -DVIDEO_PLAYER='"/mnt/SDCARD/Emus/VIDEOS/mpv.sh "' \
            -DMANUAL_READER='"/mnt/SDCARD/Apps/.PDFReader/launch.sh "'
CXXFLAGS += -O3 -fomit-frame-pointer -Ofast -flto
CXXFLAGS += -s -ffunction-sections -fdata-sections
LDFLAGS += -Wl,--gc-sections -flto
LDLIBS := $(shell sdl2-config --libs) -lsqlite3 -lSDL2_ttf -lSDL2_image
LDLIBS += -lpthread -lm -lz -ldl
