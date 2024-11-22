NAME=time_logs

SRCS= srcs/main.cpp srcs/utils.cpp srcs/DB.cpp srcs/GUI.cpp srcs/Timer.cpp
OBJS=$(patsubst srcs/%, build/%,$(SRCS:.cpp=.o))

CXX=g++
CXXFLAGS=-Wall -Wextra -std=c++17 -g `sdl2-config --cflags`
LDLIBS= `sdl2-config --libs` -lsqlite3 -lSDL2_ttf -lSDL2_image -lz
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
