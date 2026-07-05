# Compiler and base flags
CXX = clang++
CXXFLAGS = -std=c++23 -Wall -Wextra -Wshadow -pedantic -Isrc -fconstexpr-steps=500000000

# Dependency tracking flags
DEPFLAGS = -MMD -MP

# Build directories
BUILD_DIR = build
BIN_DIR = bin

# Target executable name
TARGET = $(BIN_DIR)/stargaze

# Source and object files
SOURCES = $(wildcard src/*.cpp)
OBJECTS = $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))
DEPS = $(OBJECTS:.o=.d)

# Profile flags
RELEASE_FLAGS = -O3 -flto -march=native -DNDEBUG
DEBUG_FLAGS = -O2 -g -fsanitize=address -fsanitize=undefined -DLOCAL -DDEBUG -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC

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

# --- Testing ---
TEST_DIR        = tests
TEST_BUILD_DIR  = build/tests
TEST_BIN        = $(BIN_DIR)/stargaze_tests

ENGINE_SOURCES  = $(filter-out src/main.cpp,$(SOURCES))
TEST_SOURCES    = $(wildcard tests/*.cpp tests/unit/*.cpp tests/integration/*.cpp)

# Engine + test objects, both compiled with sanitizers, into build/tests
TEST_OBJECTS    = $(patsubst src/%.cpp,$(TEST_BUILD_DIR)/src/%.o,$(ENGINE_SOURCES)) \
                  $(patsubst tests/%.cpp,$(TEST_BUILD_DIR)/%.o,$(TEST_SOURCES))
TEST_DEPS       = $(TEST_OBJECTS:.o=.d)

TEST_FLAGS      = $(DEBUG_FLAGS) -DVERIFY_CONSISTENCY -Itests

$(TEST_BUILD_DIR)/src/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(TEST_FLAGS) $(DEPFLAGS) -c $< -o $@

$(TEST_BUILD_DIR)/%.o: tests/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(TEST_FLAGS) $(DEPFLAGS) -c $< -o $@

$(TEST_BIN): $(TEST_OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(TEST_FLAGS) $(TEST_OBJECTS) -o $@

# Build the engine binary first so UCI integration tests can spawn it
test: debug $(TEST_BIN)
	./$(TEST_BIN)

test-unit: $(TEST_BIN)
	./$(TEST_BIN) --test-suite-exclude=integration

# Generate compilation database for language server (clangd)
compdb:
	python3 tools/gen_compile_commands.py

# Include dependency files if they exist
-include $(DEPS)
-include $(TEST_DEPS)

# Phony targets
.PHONY: all release debug verify run run-debug run-verify clean run-perft test test-unit compdb

# Perft execution defaults
FEN ?= "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
DEPTH ?= 5

# Run perft
run-perft: release
	@(echo "position fen $(FEN)"; echo "go perft $(DEPTH)"; echo "quit") | ./$(TARGET)
