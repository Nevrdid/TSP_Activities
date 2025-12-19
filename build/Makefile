# Default target
.DEFAULT_GOAL := all

# Source and object files
SRCS := $(wildcard ../srcs/*.cpp)
OBJS := $(patsubst ../srcs/%, objs/%,$(SRCS:.cpp=.o))

ACT_SRCS := $(wildcard ../srcs/activities/*.cpp)
ACT_OBJS := $(patsubst ../srcs/%, objs/%,$(ACT_SRCS:.cpp=.o))

TIM_SRCS := $(wildcard ../srcs/timer/*.cpp)
TIM_OBJS := $(patsubst ../srcs/%, objs/%,$(TIM_SRCS:.cpp=.o))

LDFLAGS += -I../includes/

# Configuration inclusion based on GOAL
ifeq ($(GOAL),cross_tsp)
  include config_cross_tsp.mk
else ifeq ($(GOAL),native_aarch64)
	include config_native_aarch64.mk
else
  include config_native_x86.mk
endif

# Build rules
objs/%.o: ../srcs/%.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -c $< -o $@

bin/activities$(EXT): $(OBJS) $(ACT_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

bin/timer$(EXT): $(OBJS) $(TIM_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

# Top-level targets
all: bin/activities$(EXT) bin/timer$(EXT)

tsp:
	./crosscompile.sh

aarch64:
	$(MAKE) GOAL=native_aarch64

clean:
	rm -f bin/*

fclean: clean
	rm -rf $(OBJS) $(ACT_OBJS) $(TIM_OBJS)

re: fclean all

info:
	@echo "GOAL: $(GOAL)"
	@echo "CXX: $(CXX)"
	@echo "CXXFLAGS: $(CXXFLAGS)"
	@echo "LDLIBS: $(LDLIBS)"
	@echo "LDFLAGS: $(LDFLAGS)"
	@echo "NAME: $(NAME)"
	@echo "SYSROOT: $(SYSROOT)"

.PHONY: all tsp aarch64 clean fclean re info
