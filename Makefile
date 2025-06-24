# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -O2 -I./src/include -I$(BUILD_DIR) -flto  # 增加头文件搜索路径
LDFLAGS = -larchive -flto
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

# 改进的版本信息生成规则 --------------------------------------------------------
$(BUILD_DIR)/git_version.h:
	@mkdir -p $(BUILD_DIR)
	@echo "  GEN     $@"
	@( \
		# 获取Git信息并设置默认值 \
		GIT_HASH=$$(git rev-parse --short HEAD 2>/dev/null || echo "unknown"); \
		GIT_BRANCH=$$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo "unknown"); \
		GIT_TAG=$$(git describe --tags --abbrev=0 2>/dev/null || echo "untagged"); \
		if [ -n "$$(git status --porcelain 2>/dev/null)" ]; then \
			GIT_DIRTY="-dirty"; \
		else \
			GIT_DIRTY=""; \
		fi; \
		BUILD_DATE=$$(date +'%Y-%m-%d %H:%M:%S'); \
		\
		# 生成临时文件 \
		echo "#ifndef GIT_VERSION_H" > $@.tmp; \
		echo "#define GIT_VERSION_H" >> $@.tmp; \
		echo "#define GIT_COMMIT_HASH \"$${GIT_HASH}$${GIT_DIRTY}\"" >> $@.tmp; \
		echo "#define GIT_BRANCH \"$${GIT_BRANCH}\"" >> $@.tmp; \
		echo "#define GIT_TAG \"$${GIT_TAG}\"" >> $@.tmp; \
		echo "#define BUILD_TIMESTAMP \"$${BUILD_DATE}\"" >> $@.tmp; \
		echo "#endif" >> $@.tmp; \
		\
		# 仅当内容变化时更新文件 \
		if ! cmp -s $@.tmp $@; then \
			mv $@.tmp $@; \
		else \
			rm $@.tmp; \
		fi \
	)
# -----------------------------------------------------------------------------

# Compile and generate dependencies
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(BUILD_DIR)/git_version.h
	@mkdir -p $(BUILD_DIR)
	@echo "  CC      $@"
	@$(CC) $(CFLAGS) -MD -MP -c $< -o $@

# Include generated dependencies
-include $(DEPS)

clean:
	@echo "  CLEAN"
	@rm -rf $(BUILD_DIR) $(TARGET)

install: $(TARGET)
	@echo "  INSTALL $(TARGET)"
	@install -m 755 $(TARGET) /usr/local/bin/

help:
	@echo "Build targets:"
	@echo "  all       - Build project (default)"
	@echo "  clean     - Remove build artifacts"
	@echo "  install   - Install to /usr/local/bin"