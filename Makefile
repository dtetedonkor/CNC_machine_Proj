# Parameters
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g   # Enable warnings and debugging info
BUILD_DIR = build                    # Build directory for object files
SRC_DIR = src                        # Directory containing source files
BIN_DIR = bin                        # Directory for binary output
TARGET = $(BIN_DIR)/engraver         # Final executable

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.c)    # All .c files in src folder
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS)) # Corresponding object files

# Default target
all: dirs $(TARGET)

# Link target binary
$(TARGET): $(OBJS)
	@echo "Linking $@..."
	$(CC) $(CFLAGS) -o $@ $^

# Compile object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Create build directories
dirs:
	@mkdir -p $(BUILD_DIR) $(BIN_DIR)

# Test target (for separate testing Makefile)
test:
	$(MAKE) -C tests all

# Clean files
clean:
	@echo "Cleaning up..."
	@rm -rf $(BUILD_DIR) $(BIN_DIR)

.PHONY: all clean dirs test
