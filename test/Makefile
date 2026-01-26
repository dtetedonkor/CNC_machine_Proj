# Parameters
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
TEST_DIR = tests
SRC_DIR = src
BUILD_DIR = $(TEST_DIR)/build
BIN_DIR = $(TEST_DIR)/bin
TEST_TARGET = $(BIN_DIR)/test_runner

# Source files for tests and the application (mock the main executable)
SRCS = $(wildcard $(SRC_DIR)/*.c $(TEST_DIR)/*.c)          # All source files
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(filter $(SRC_DIR)/%.c,$(SRCS))) \
       $(patsubst $(TEST_DIR)/%.c, $(BUILD_DIR)/%.o, $(filter $(TEST_DIR)/%.c,$(SRCS)))

# Default target
all: dirs $(TEST_TARGET)

# Link and build the test runner target
$(TEST_TARGET): $(OBJS)
	@echo "Linking $@..."
	$(CC) $(CFLAGS) -o $@ $^

# Compile object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	@echo "Compiling $< (source)..."
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(TEST_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	@echo "Compiling $< (test)..."
	$(CC) $(CFLAGS) -c $< -o $@

# Create build directories
dirs:
	@mkdir -p $(BUILD_DIR) $(BIN_DIR)

# Test execution
run: all
	@echo "Running tests..."
	$(TEST_TARGET)

# Clean files
clean:
	@echo "Cleaning up test files..."
	@rm -rf $(BUILD_DIR) $(BIN_DIR)

.PHONY: all clean dirs run
