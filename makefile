# Compiler and base flags
CXX = clang++
CXXFLAGS = -std=c++23 -Wall -Wextra -Wshadow -pedantic -Isrc

# Dependency tracking flags
DEPFLAGS = -MMD -MP

# Build directories
BUILD_DIR          = build
BUILD_RELEASE_DIR  = build/release
BUILD_DEBUG_DIR    = build/debug
BUILD_SANITIZE_DIR = build/sanitize
TEST_BUILD_DIR     = build/tests
BIN_DIR            = bin

# Target executable name
TARGET = $(BIN_DIR)/stargaze

# Source and object files
SOURCES = $(wildcard src/*.cpp)

RELEASE_OBJECTS  = $(patsubst src/%.cpp,$(BUILD_RELEASE_DIR)/%.o,$(SOURCES))
DEBUG_OBJECTS    = $(patsubst src/%.cpp,$(BUILD_DEBUG_DIR)/%.o,$(SOURCES))
SANITIZE_OBJECTS = $(patsubst src/%.cpp,$(BUILD_SANITIZE_DIR)/%.o,$(SOURCES))

RELEASE_DEPS  = $(RELEASE_OBJECTS:.o=.d)
DEBUG_DEPS    = $(DEBUG_OBJECTS:.o=.d)
SANITIZE_DEPS = $(SANITIZE_OBJECTS:.o=.d)

# Profile flags
RELEASE_FLAGS  = -O3 -flto -march=native -DNDEBUG
DEBUG_FLAGS    = -O2 -g -fsanitize=undefined -DLOCAL -DDEBUG -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC
SANITIZE_FLAGS = $(DEBUG_FLAGS) -fsanitize=address

# Default profile is release
all: release

# Release target
release: $(RELEASE_OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(RELEASE_FLAGS) -flto $(RELEASE_OBJECTS) -o $(TARGET)

# Debug target
debug: $(DEBUG_OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(DEBUG_FLAGS) $(DEBUG_OBJECTS) -o $(TARGET)

# Verification target (debug with consistency verification checks)
verify: DEBUG_FLAGS += -DVERIFY_CONSISTENCY
verify: debug

# Dedicated sanitize target (with AddressSanitizer)
sanitize: $(SANITIZE_OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) $(SANITIZE_OBJECTS) -o $(TARGET)

# Compile object files for each profile
$(BUILD_RELEASE_DIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(RELEASE_FLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD_DEBUG_DIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEBUG_FLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD_SANITIZE_DIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) $(DEPFLAGS) -c $< -o $@

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
TEST_BIN        = $(BIN_DIR)/stargaze_tests

ENGINE_SOURCES  = $(filter-out src/main.cpp,$(SOURCES))
TEST_SOURCES    = $(wildcard tests/*.cpp tests/unit/*.cpp tests/integration/*.cpp)
BENCHMARK_SOURCES = $(wildcard tests/benchmark/*.cpp)
BENCHMARK_BUILD_DIR = build/benchmarks
BENCHMARK_BIN = $(BIN_DIR)/stargaze_benchmarks
BENCHMARK_OBJECTS = $(patsubst src/%.cpp,$(BENCHMARK_BUILD_DIR)/src/%.o,$(ENGINE_SOURCES)) \
                    $(patsubst tests/%.cpp,$(BENCHMARK_BUILD_DIR)/%.o,$(BENCHMARK_SOURCES)) \
                    $(BENCHMARK_BUILD_DIR)/main.o
BENCHMARK_DEPS = $(BENCHMARK_OBJECTS:.o=.d)

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

$(BENCHMARK_BUILD_DIR)/src/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(RELEASE_FLAGS) -Itests $(DEPFLAGS) -c $< -o $@

$(BENCHMARK_BUILD_DIR)/%.o: tests/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(RELEASE_FLAGS) -Itests $(DEPFLAGS) -c $< -o $@

$(BENCHMARK_BUILD_DIR)/main.o: tests/main.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(RELEASE_FLAGS) -Itests $(DEPFLAGS) -c $< -o $@

$(BENCHMARK_BIN): $(BENCHMARK_OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(RELEASE_FLAGS) -flto $(BENCHMARK_OBJECTS) -o $@

# Build the engine binary first so UCI integration tests can spawn it
test: debug $(TEST_BIN)
	./$(TEST_BIN)

test-unit: $(TEST_BIN)
	./$(TEST_BIN) --test-suite-exclude=integration

benchmark: $(BENCHMARK_BIN)
	./$(BENCHMARK_BIN)

# Generate compilation database for language server (clangd)
compdb:
	python3 tools/gen_compile_commands.py

# Run a strength match against Stockfish
STOCKFISH ?= stockfish
SKILL ?= 0
GAMES ?= 48
BASE ?= 10
INCREMENT ?= 0.1
PGN ?= games/stockfish-match.pgn

match-stockfish: release
	python3 tools/match_stockfish.py --stockfish "$(STOCKFISH)" --engine "./$(TARGET)" \
		--skill "$(SKILL)" --games "$(GAMES)" --base "$(BASE)" \
		--increment "$(INCREMENT)" --pgn "$(PGN)" $(MATCH_ARGS)

# Include dependency files if they exist
-include $(RELEASE_DEPS)
-include $(DEBUG_DEPS)
-include $(SANITIZE_DEPS)
-include $(TEST_DEPS)
-include $(BENCHMARK_DEPS)

# Phony targets
.PHONY: all release debug verify sanitize run run-debug run-verify clean run-perft test test-unit benchmark compdb match-stockfish

# Perft execution defaults
FEN ?= "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
DEPTH ?= 5

# Run perft
run-perft: release
	@(echo "position fen $(FEN)"; echo "go perft $(DEPTH)"; echo "quit") | ./$(TARGET)
