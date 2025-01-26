
SRCS=$(wildcard srcs/*.cpp)
OBJS=$(patsubst srcs/%, build/%,$(SRCS:.cpp=.o))

# Default compiler and flags
CXX=g++
CXXFLAGS=-Wall -Wextra -std=c++17 `sdl2-config --cflags`

ifeq ($(TEST), 1)
	NAME=activities.x86
	CXXFLAGS+=-g
	DEF='-DAPP_DIR="./"' '-DVIDEO_PLAYER="/usr/bin/ffplay "' '-DMANUAL_READER="/usr/bin/okular "' '-DUSE_KEYBOARD'
else
	NAME=activities
	CXXFLAGS+=-march=armv8-a+crc+simd+crypto -mtune=cortex-a53 -mcpu=cortex-a53+crc -O3
	DEF='-DAPP_DIR="/mnt/SDCARD/Apps/Activities/"' '-DVIDEO_PLAYER="/mnt/SDCARD/Emus/VIDEOS/mpv.sh"' '-DMANUAL_READER="/mnt/SDCARD/Apps/.PDFReader/launch.sh"'
endif

CXXFLAGS+=$(DEF)

LDLIBS= `sdl2-config --libs` -lsqlite3 -lSDL2_ttf -lSDL2_image
LDFLAGS= -Iincludes/

build/%.o: srcs/%.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -c $< -o $@

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS) $(LDLIBS)

all: $(NAME)

clean:
	rm -f $(NAME)

fclean: clean
	rm -f $(OBJS)

re: fclean all

.PHONY: clean fclean re all
