#include "chd.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static int check_proot(void) {
  if (execute_command("proot >/dev/null 2>&1") == 127) {
    printc(FG_RED, BG_DEFAULT, STYLE_RESET, "You have not install proot.\n");
    return -1;
  }
  return 0;
}

void list_installed_containers(void) {
  DIR *dir = opendir(cfg.cfg_path);
  if (!dir) {
    fprintf(stderr, "Error: Unable to open containers directory at %s\n",
            cfg.cfg_path);
    exit(EXIT_FAILURE);
  }

  printf("Available containers:\n");
  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    // Skip "." and ".."
    if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0 &&
        strcmp(entry->d_name, ".tmp") != 0) {
      printf("  - %s\n", entry->d_name);
    }
  }
  closedir(dir);
}

char *find_container_path(const char *name) {
  static char path[PATH_MAX];
  snprintf(path, sizeof(path), "%s/%s", cfg.cfg_path, name);

  struct stat st;
  if (stat(path, &st) == -1 || !S_ISDIR(st.st_mode)) {
    return NULL;
  }

  return strdup(path);
}

void run_proot_container(const char *container_name) {
  struct stat st;
  if (check_proot() == -1) {
    goto EXIT;
  }

  char *container_path;
  if (container_name == NULL || strlen(container_name) == 0) {
    printf("No container specified. Searching for installed containers...\n");
    list_installed_containers();
    exit(EXIT_SUCCESS);
  } else {
    container_path = find_container_path(container_name);
    if (container_path == NULL) {
      printc(FG_RED, BG_DEFAULT, STYLE_RESET, "Error");
      exit(EXIT_FAILURE);
    }
  }
  static char path[PATH_MAX];
  static char sh[128] = {0};
  snprintf(path, sizeof(path), "%s/%s", container_path, "bin/bash");
  printf("%s\n", path);
  if (lstat(path, &st) == 0 || !S_ISDIR(st.st_mode)) {
    snprintf(sh, sizeof(sh), "%s", "/bin/bash");
  } else {
    memset(path, 0x00, sizeof(path));
    snprintf(path, sizeof(path), "%s/%s", container_path, "bin/sh");
    if (lstat(path, &st) == 0 || !S_ISDIR(st.st_mode)) {
      snprintf(sh, sizeof(sh), "%s", "/bin/sh");
    } else {
      printf("%s\n", path);
      printc(FG_RED, BG_DEFAULT, STYLE_RESET, "Unknown shell!");
      return;
    }
  }

  char proot_cmd[2048] = {0};
  snprintf(proot_cmd, sizeof(proot_cmd),
           "proot --link2symlink -0 -r %s -b /dev -b /proc -b %s/root:/dev/shm "
           "-w /root /usr/bin/env -i HOME=/root "
           "PATH=/usr/local/sbin:/usr/local/bin:/bin:/sbin:/usr/sbin:/usr/"
           "games:/usr/local/games "
           "SHELL=%s TERM=$TERM LANG=C.UTF-8 %s --login",
           container_path, container_path, sh, sh);
  printf("%s\n", proot_cmd);
  free(container_path);
  if (execute_command(proot_cmd) == -1) {
  EXIT:
    fprintf(stderr,
            "Failed to execute `%s`\nexecv() returned: %d\nerror reason: "
            "%s\nNote: unset $LD_PRELOAD before running may fix this issue.\n",
            proot_cmd, errno, strerror(errno));
  }
}