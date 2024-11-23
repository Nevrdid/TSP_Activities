NAME=activities

SRCS= srcs/main.cpp srcs/utils.cpp srcs/DB.cpp srcs/GUI.cpp srcs/Timer.cpp
OBJS=$(patsubst srcs/%, build/%,$(SRCS:.cpp=.o))

CXX=g++


ifeq ($(TEST), 1)
	APP_DIR='-DAPP_DIR="./"'
	CXXFLAGS=-Wall -Wextra -std=c++17 `sdl2-config --cflags` -g $(APP_DIR)
else
	APP_DIR='-DAPP_DIR="/mnt/SDCARD/Apps/Activities/"'
	CXXFLAGS=-Wall -Wextra -std=c++17 `sdl2-config --cflags` -O3 $(APP_DIR)
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
