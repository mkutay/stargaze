# Compiler and base flags
CXX = clang++
CXXFLAGS = -std=c++23 -Wall -Wextra -Wshadow -pedantic -Isrc

# Dependency tracking flags
DEPFLAGS = -MMD -MP

# Build directories
BUILD_DIR = build
BIN_DIR = bin

# Target executable name
TARGET = $(BIN_DIR)/stargaze

# Source and object files
SOURCES = $(wildcard src/*.cpp)
CORE_SOURCES = $(filter-out src/perft.cpp, $(filter-out src/main.cpp, $(SOURCES)))

MAIN_SOURCES = $(CORE_SOURCES) src/main.cpp
OBJECTS = $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(MAIN_SOURCES))
DEPS = $(OBJECTS:.o=.d)

# Profile flags
RELEASE_FLAGS = -O3 -flto -march=native -DNDEBUG
DEBUG_FLAGS = -g -fsanitize=address -fsanitize=undefined -DLOCAL -DDEBUG -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC

# Default profile is release
all: release

# Release target
release: CXXFLAGS += $(RELEASE_FLAGS)
release: LDFLAGS += -flto
release: $(TARGET)

# Debug target
debug: CXXFLAGS += $(DEBUG_FLAGS)
debug: $(TARGET)

# Verification target (debug with consistency verification checks)
verify: CXXFLAGS += $(DEBUG_FLAGS) -DVERIFY_CONSISTENCY
verify: $(TARGET)

# Link the executable
$(TARGET): $(OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $@ $(LDFLAGS)

# Compile object files
$(BUILD_DIR)/%.o: src/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

# Run the executable (defaults to building release first)
run: release
	./$(TARGET)

# Run the debug executable
run-debug: debug
	./$(TARGET)

# Run the verification executable
run-verify: verify
	./$(TARGET)

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

# Include dependency files if they exist
-include $(DEPS)

# Phony targets
.PHONY: all release debug verify run run-debug run-verify clean perft run-perft

# Perft execution defaults
FEN ?= "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
DEPTH ?= 5

# Run perft
run-perft: perft
	./bin/perft $(FEN) $(DEPTH) --divide

# Perft target
PERFT_SOURCES = $(CORE_SOURCES) src/perft.cpp
PERFT_OBJECTS = $(patsubst src/%.cpp,$(BUILD_DIR)/perft_%.o,$(PERFT_SOURCES))

perft: CXXFLAGS += $(RELEASE_FLAGS)
perft: LDFLAGS += -flto
perft: $(BIN_DIR)/perft

$(BIN_DIR)/perft: $(PERFT_OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(PERFT_OBJECTS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/perft_%.o: src/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@
