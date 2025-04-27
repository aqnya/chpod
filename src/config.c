#include "chd.h"

#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* 全局变量定义 */
char config_tmp[PATH_MAX] = {0};
char config_h[PATH_MAX] = {0}; // 配置路径缓冲区
char *v_home = NULL;           // HOME环境变量值

static int mkdir_p(const char *path, mode_t mode) {
  char tmp[PATH_MAX];
  char *p = NULL;
  size_t len;

  snprintf(tmp, sizeof(tmp), "%s", path);
  len = strlen(tmp);

  /* 去除末尾斜杠 */
  if (tmp[len - 1] == '/') {
    tmp[len - 1] = 0;
  }

  for (p = tmp + 1; *p; p++) {
    if (*p == '/') {
      *p = 0;
      if (mkdir(tmp, mode) && errno != EEXIST) {
        return -1;
      }
      *p = '/';
    }
  }

  if (mkdir(tmp, mode) && errno != EEXIST) {
    return -1;
  }
  return 0;
}

static int ensure_config_dirs(int count, ...) {
  va_list args;
  va_start(args, count);

  for (int i = 0; i < count; i++) {
    const char *dir = va_arg(args, const char *);

    /* 路径有效性检查 */
    if (strlen(dir) >= PATH_MAX) {
      cprintf(RED, "[ERROR] 路径过长: %s\n", dir);
      va_end(args);
      return CHD_ERR_INVALID_ARG;
    }

    /* 目录存在性检查 */
    struct stat st;
    if (stat(dir, &st) == 0) {
      if (S_ISDIR(st.st_mode)) {
        //    cprintf(CYAN, "[OK] 目录已存在: %s\n", dir);
        continue;
      }
      cprintf(RED, "[ERROR] %s is file\n", dir);
      va_end(args);
      return CHD_ERR_IO;
    }

    /* 创建目录 */
    if (mkdir_p(dir, 0755) != 0) {
      cperror(RED, "[FATAL] creat failed\n");
      va_end(args);
      return CHD_ERR_IO;
    }
    //   cprintf(GREEN, "[OK] 已创建目录: %s\n", dir);
  }

  va_end(args);
  return CHD_SUCCESS;
}

static int build_config_path(char *buf, const char *format, ...) {
  va_list args;
  va_start(args, format);

  int ret = vsnprintf(buf, PATH_MAX, format, args);
  va_end(args);

  if (ret < 0) {
    cperror(RED, "[FATAL] format path failed");
    return CHD_ERR_IO;
  } else if (ret >= PATH_MAX) {
    cperror(RED, "[FATAL] path too long");
    return CHD_ERR_INVALID_ARG;
  }

  return CHD_SUCCESS;
}

static void unset_ld_preload(void) {
    // Unset the LD_PRELOAD environment variable
    if (unsetenv("LD_PRELOAD") != 0) {
        perror("Failed to unset LD_PRELOAD");
    } else {
        printf("LD_PRELOAD successfully unset.\n");
    }
}

void config_init(void) {
  /* 环境变量检查 */
  const char *env_home = getenv("HOME");
  const char *env_prefix = getenv("PREFIX");

  if (!env_home || !env_prefix) {
    cperror(RED, "[FATAL] necessary: HOME, PREFIX");
    exit(EXIT_FAILURE);
  }
unset_ld_preload();
  /* 复制HOME值 */
  if (!(v_home = strdup(env_home))) {
    cperror(RED, "[FATAL] HOME memory alloc failed");
    exit(EXIT_FAILURE);
  }

  /* 生成配置路径 */
  int status = CHD_SUCCESS;

  // 生成主配置目录路径（使用全局变量 config_h）
  status = build_config_path(config_h, "%s/%s", v_home, ".chd");
  if (status != CHD_SUCCESS) {
    config_cleanup();
    exit(EXIT_FAILURE);
  }

  // 生成临时目录路径（使用全局变量 config_tmp）
  status = build_config_path(config_tmp, "%s/%s", config_h, ".tmp");
  if (status != CHD_SUCCESS) {
    config_cleanup();
    exit(EXIT_FAILURE);
  }

  // 创建目录结构
  status = ensure_config_dirs(2, config_h, config_tmp);
  if (status != CHD_SUCCESS) {
    config_cleanup();
    exit(EXIT_FAILURE);
  }
}

void config_cleanup(void) {
  if (v_home) {
    free(v_home);
    v_home = NULL;
  }

  memset(config_h, 0, PATH_MAX); // 清空配置路径
}