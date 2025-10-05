CXX = g++
CXXFLAGS = -std=c++23 -Wall -Wextra -Wshadow -O2 -pedantic
DEBUGFLAGS = -fsanitize=address -fsanitize=undefined -DLOCAL -DDEBUG -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC
CXXFLAGS += $(DEBUGFLAGS)

SOURCES = $(wildcard src/*.cpp)

all: bin/stargaze

bin/stargaze: $(SOURCES)
	mkdir -p bin
	$(CXX) $(CXXFLAGS) $^ -o $@

run: all
	./bin/stargaze