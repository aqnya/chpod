# Compiler and flags
CC = gcc
CFLAGS = -Wall -Werror -Wextra -O2 -I./src/include -I$(BUILD_DIR)
LDFLAGS = -larchive -flto
TARGET = chpod

# Directory structure
SRC_DIR = src
BUILD_DIR = build
INCLUDE_DIR = $(SRC_DIR)/include

# Source and object files
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))
DEPS = $(OBJS:.o=.d)

# Header files (auto-detected)
LOCAL_HEADERS := $(wildcard $(INCLUDE_DIR)/*.h)
SYS_HEADERS := archive.h

.PHONY: all clean install help check_headers

all: check_headers $(TARGET)

# Check for required local and system headers
check_headers:
	@echo "  CHECK   headers"
	@missing=0; \
	for hdr in $(LOCAL_HEADERS); do \
		if [ ! -f "$$hdr" ]; then \
			echo "Missing header: $$hdr"; \
			missing=1; \
		fi; \
	done; \
	for shdr in $(SYS_HEADERS); do \
		echo '#include <'$$shdr'>' | $(CC) -E - > /dev/null 2>&1 || { \
			echo "Missing header: $$shdr"; \
			missing=1; \
		}; \
	done; \
	if [ $$missing -eq 1 ]; then \
		echo "Header check failed."; exit 1; \
	fi

# Main build target
$(TARGET): $(OBJS)
	@echo "  LINK    $@"
	@$(CC) $^ -o $@ $(LDFLAGS)

# Auto-generate git version header
$(BUILD_DIR)/git_version.h:
	@mkdir -p $(BUILD_DIR)
	@echo "  GEN     $@"
	@( \
		GIT_HASH=$$(git rev-parse --short HEAD 2>/dev/null || echo "unknown"); \
		GIT_BRANCH=$$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo "unknown"); \
		GIT_TAG=$$(git describe --tags --abbrev=0 2>/dev/null || echo "untagged"); \
		if [ -n "$$(git status --porcelain 2>/dev/null)" ]; then \
			GIT_DIRTY="-dirty"; \
		else \
			GIT_DIRTY=""; \
		fi; \
		BUILD_DATE=$$(date +'%Y-%m-%d %H:%M:%S'); \
		echo "#ifndef GIT_VERSION_H" > $@.tmp; \
		echo "#define GIT_VERSION_H" >> $@.tmp; \
		echo "#define GIT_COMMIT_HASH \"$${GIT_HASH}$${GIT_DIRTY}\"" >> $@.tmp; \
		echo "#define GIT_BRANCH \"$${GIT_BRANCH}\"" >> $@.tmp; \
		echo "#define GIT_TAG \"$${GIT_TAG}\"" >> $@.tmp; \
		echo "#define BUILD_TIMESTAMP \"$${BUILD_DATE}\"" >> $@.tmp; \
		echo "#endif" >> $@.tmp; \
		if ! cmp -s $@.tmp $@; then mv $@.tmp $@; else rm $@.tmp; fi \
	)

# Compile and generate dependencies
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(BUILD_DIR)/git_version.h
	@mkdir -p $(BUILD_DIR)
	@echo "  CC      $@"
	@$(CC) $(CFLAGS) -MD -MP -c $< -o $@

# Include auto-generated dependency files
-include $(DEPS)

# Clean build artifacts
clean:
	@echo "  CLEAN"
	@rm -rf $(BUILD_DIR) $(TARGET)

# Install binary
install: $(TARGET)
	@echo "  INSTALL $(TARGET)"
	@install -m 755 $(TARGET) /usr/local/bin/

# Help
help:
	@echo "Build targets:"
	@echo "  all       - Build project (default)"
	@echo "  clean     - Remove build artifacts"
	@echo "  install   - Install to /usr/local/bin"
	@echo "  help      - Show this help message"