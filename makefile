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
OBJECTS = $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))
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
.PHONY: all release debug verify run run-debug run-verify clean
