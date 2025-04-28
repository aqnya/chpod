# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -O2 -g -I./src/include -g
LDFLAGS = -lcrypto -lssl -larchive
TARGET = chd

# Directory structure
SRC_DIR = src
BUILD_DIR = build

# Source and object files
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))
DEPS = $(OBJS:.o=.d)

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $^ -o $@ $(LDFLAGS)

# Compile and generate dependencies
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -MD -MP -c $< -o $@

# Include generated dependencies
-include $(DEPS)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/

# Add help target to match common conventions
help:
	@echo "Build targets:"
	@echo "  all       - Build project (default)"
	@echo "  clean     - Remove build artifacts"
	@echo "  install   - Install to /usr/local/bin"
