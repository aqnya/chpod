#include "chd.h"

#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

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
      printc(FG_RED, BG_DEFAULT, STYLE_RESET, "[ERROR] Path too long: %s\n",
             dir);
      va_end(args);
      return CHD_ERR_INVALID_ARG;
    }

    /* 目录存在性检查 */
    struct stat st;
    if (stat(dir, &st) == 0) {
      if (S_ISDIR(st.st_mode)) {
        continue;
      }
      printc(FG_RED, BG_DEFAULT, STYLE_RESET, "[ERROR] %s is file\n", dir);
      va_end(args);
      return CHD_ERR_IO;
    }

    /* 创建目录 */
    if (mkdir_p(dir, 0755) != 0) {
      printc(FG_RED, BG_DEFAULT, STYLE_RESET, "[FATAL] creat failed\n");
      va_end(args);
      return CHD_ERR_IO;
    }
  }

  va_end(args);
  return CHD_SUCCESS;
}

static void unset_ld_preload(void) {
  if (unsetenv("LD_PRELOAD") != 0) {
    printc(FG_RED, BG_DEFAULT, STYLE_RESET,
           "[FATAL] Failed to unset LD_PRELOAD");
  }
}

void config_init(void) {
  /* 环境变量检查 */
  const char *env_home = getenv("HOME");
  const char *env_prefix = getenv("PREFIX");

  if (!env_home || !env_prefix) {
    printc(FG_RED, BG_DEFAULT, STYLE_RESET, "[FATAL] necessary: HOME, PREFIX");
    exit(EXIT_FAILURE);
  }
  unset_ld_preload();
  
strncpy(cfg.home,getenv("HOME"),PATH_MAX -1);
  /* 生成配置路径 */
  int status = CHD_SUCCESS;

  snprintf(cfg.cfg_path,sizeof(cfg.cfg_path),"%s/%s",cfg.home,".chd");

snprintf(cfg.tmp_dir,sizeof(cfg.tmp_dir),"%s/%s",cfg.cfg_path,".tmp");

  // 创建目录结构
  status = ensure_config_dirs(2, cfg.cfg_path, cfg.tmp_dir);
  if (status != CHD_SUCCESS) {
    config_cleanup();
    exit(EXIT_FAILURE);
  }
}

void config_cleanup(void) {

  memset(cfg.cfg_path, 0, PATH_MAX); // 清空配置路径
}