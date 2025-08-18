NAME := activities
CROSS_COMPILE := /opt/aarch64-linux-gnu-7.5.0-linaro/bin/aarch64-linux-gnu-
SYSROOT := /opt/aarch64-linux-gnu-7.5.0-linaro/sysroot
TSP_LIB_PATH := /root/workspace/minui-presenter/platform/tg5040/lib

CXX := $(CROSS_COMPILE)g++

CXXFLAGS := -Wall -Wextra -std=c++17 -O3 -fomit-frame-pointer -Ofast -flto
CXXFLAGS += --sysroot=$(SYSROOT) -I$(SYSROOT)/usr/include/SDL2
CXXFLAGS += -DAPP_DIR=\"/mnt/SDCARD/Apps/Activities/\" \
            -DVIDEO_PLAYER='"/mnt/SDCARD/Emus/VIDEOS/mpv.sh "' \
            -DMANUAL_READER='"/mnt/SDCARD/Apps/.PDFReader/launch.sh "'
CXXFLAGS += -s -ffunction-sections -fdata-sections

LDFLAGS += -Wl,--gc-sections -flto
LDLIBS := -L$(TSP_LIB_PATH) -L$(SYSROOT)/usr/lib
LDLIBS += -lSDL2 -lSDL2_image -lSDL2_ttf -lsqlite3 -lpthread -lm -lz -ldl -lstdc++fs
