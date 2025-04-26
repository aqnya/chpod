# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -Werror -g -Iinclude -DDEBUG
LDFLAGS = -lcrypto -larchive

# Project structure
SRC_DIR = src
BUILD_DIR = build
TARGET = chd

# Source files (auto-discover)
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))
DEPS = $(OBJS:.o=.d)

# Create build directory if not exists
$(shell mkdir -p $(BUILD_DIR))

# Default target
all: $(TARGET)

# Main executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Compile with dependency generation
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# Include auto-generated dependencies
-include $(DEPS)

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

# Install to /usr/local/bin
install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin

# Uninstall
uninstall:
	rm -f /usr/local/bin/$(TARGET)

# Phony targets
.PHONY: all clean install uninstall
