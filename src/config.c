#include "chd.h"

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static int __mkdir_r(const char *path, mode_t mode) {
  char tmp[PATH_MAX];
  size_t len = strlen(path);

  if (len >= PATH_MAX)
    return -1;

  strncpy(tmp, path, PATH_MAX);
  tmp[len] = '\0';

  for (char *p = tmp + 1; *p; p++) {
    if (*p == '/') {
      *p = '\0';
      if (mkdir(tmp, mode) && errno != EEXIST)
        return -1;
      *p = '/';
    }
  }

  return mkdir(tmp, mode) && errno != EEXIST ? -1 : 0;
}

static void unset_ld_preload(void) { unsetenv("LD_PRELOAD"); }

void config_init(void) {
  const char *env_home = getenv("HOME");
  const char *env_prefix = getenv("PREFIX");

  if (!env_home || !env_prefix) {
    plog(LOG_ERROR, "Required Environment Variables(HOME,PREFIX)");
    exit(EXIT_FAILURE);
  }

  unset_ld_preload();

  strncpy(cfg.home, env_home, PATH_MAX - 1);
  snprintf(cfg.cfg_path, sizeof(cfg.cfg_path), "%s/.chd", cfg.home);
  snprintf(cfg.tmp_dir, sizeof(cfg.tmp_dir), "%s/.tmp", cfg.cfg_path);

  if (strlen(cfg.cfg_path) >= PATH_MAX || strlen(cfg.tmp_dir) >= PATH_MAX) {
    plog(LOG_ERROR, "Path too long\n");
    exit(EXIT_FAILURE);
  }

  if (__mkdir_r(cfg.cfg_path, 0755) != 0 || __mkdir_r(cfg.tmp_dir, 0755) != 0) {
    plog(LOG_ERROR, "Failed to create dirs\n");
    config_cleanup();
    exit(EXIT_FAILURE);
  }
}

void config_cleanup(void) { memset(cfg.cfg_path, 0, PATH_MAX); }