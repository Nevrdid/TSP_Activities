NAME=activities

SRCS= srcs/main.cpp srcs/utils.cpp srcs/DB.cpp srcs/GUI.cpp srcs/Timer.cpp srcs/App.cpp srcs/Config.cpp
OBJS=$(patsubst srcs/%, build/%,$(SRCS:.cpp=.o))

CXX=g++


ifeq ($(TEST), 1)
	DEF='-DAPP_DIR="./"' '-DVIDEO_PLAYER="/usr/bin/ffplay "' '-DMANUAL_READER="/usr/bin/okular "' '-DUSE_KEYBOARD'
	CXXFLAGS=-Wall -Wextra -std=c++17 `sdl2-config --cflags` -g $(DEF)
else
	DEF='-DAPP_DIR="/mnt/SDCARD/Apps/Activities/"' '-DVIDEO_PLAYER="/mnt/SDCARD/Emus/VIDEOS/mpv.sh"' '-DMANUAL_READER="/mnt/SDCARD/Apps/PDFPlayer/launch.sh"'
	CXXFLAGS=-Wall -Wextra -flto -O3 -std=c++17 `sdl2-config --cflags` $(DEF)
endif

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
