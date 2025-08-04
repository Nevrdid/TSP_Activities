SRCS=$(wildcard srcs/*.cpp)
OBJS=$(patsubst srcs/%, build/%,$(SRCS:.cpp=.o))

# Cross-compilation toolchain for TrimUI TSP (TG5040)
CROSS_PREFIX=/opt/aarch64-linux-gnu-7.5.0-linaro/bin/aarch64-linux-gnu-
SYSROOT=/opt/aarch64-linux-gnu-7.5.0-linaro/sysroot
TSP_LIB_PATH=/root/workspace/minui-presenter/platform/tg5040/lib

# Default compiler and flags
ifeq ($(TEST), 1)
	# Native compilation for testing
	CXX=g++
	NAME=activities.x86
	CXXFLAGS=-Wall -Wextra -std=c++17 -g `sdl2-config --cflags`
	DEF='-DAPP_DIR="./"' '-DVIDEO_PLAYER="/usr/bin/ffplay "' '-DMANUAL_READER="/usr/bin/okular "' '-DUSE_KEYBOARD'
	LDLIBS=`sdl2-config --libs` -lsqlite3 -lSDL2_ttf -lSDL2_image
else
	# Cross-compilation for TrimUI TSP
	CXX=$(CROSS_PREFIX)g++
	NAME=activities
	CXXFLAGS=-Wall -Wextra -std=c++17 -O3 -fomit-frame-pointer -Ofast -flto
	CXXFLAGS+=--sysroot=$(SYSROOT)
	CXXFLAGS+=-I$(SYSROOT)/usr/include/SDL2
	CXXFLAGS+=-DPLATFORM=\"tg5040\" -DUSE_SDL2 -DUSE_EXPERIMENTAL_FILESYSTEM
	DEF='-DAPP_DIR="/mnt/SDCARD/Apps/Activities/"' '-DVIDEO_PLAYER="/mnt/SDCARD/Emus/VIDEOS/mpv.sh"' '-DMANUAL_READER="/mnt/SDCARD/Apps/.PDFReader/launch.sh"'
	
	# Libraries for cross-compilation
	LDLIBS=-L$(TSP_LIB_PATH) -L$(SYSROOT)/usr/lib
	LDLIBS+=-lSDL2 -lSDL2_image -lSDL2_ttf -lsqlite3 -lpthread -lm -lz -ldl -lstdc++fs
	
	# Set library path for runtime
	export LD_LIBRARY_PATH=$(TSP_LIB_PATH)
endif

CXXFLAGS+=$(DEF)

LDFLAGS=-Iincludes/

# Create build directory if it doesn't exist
build/%.o: srcs/%.cpp | build
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -c $< -o $@

build:
	mkdir -p build

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS) $(LDLIBS)

all: $(NAME)

# Target specific for TrimUI TSP
tsp: 
	$(MAKE) all

# Target for testing (native compilation)
test:
	$(MAKE) all TEST=1

clean:
	rm -f $(NAME) activities.x86

fclean: clean
	rm -rf build/

re: fclean all

# Show compilation variables (for debugging)
info:
	@echo "CXX: $(CXX)"
	@echo "CXXFLAGS: $(CXXFLAGS)"
	@echo "LDLIBS: $(LDLIBS)"
	@echo "NAME: $(NAME)"
	@echo "SYSROOT: $(SYSROOT)"

.PHONY: clean fclean re all tsp test info
