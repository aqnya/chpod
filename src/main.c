#include "chd.h"
#include "git_version.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

cfg_head cfg = {0};

int prase_args(int argc, char *argv[]) {
  if (argc < 2) {
    show_help(argv[0]);
    return -1;
  }
  if (strcmp(argv[1], "-i") == 0 || strcmp(argv[1], "--install") == 0) {
    if (argc >= 4) {
      pull(argv[2], argv[3]);
    } else {
      plog(LOG_ERROR, "Missing parameters");
      show_help(argv[0]);
      exit(0);
    }
  } else if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
    show_help(argv[0]);
  } else if (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0) {
    printf("Hash :%s\n", GIT_COMMIT_HASH);
    printf("Branch :%s\n", GIT_BRANCH);
    printf("Tag :%s\n", GIT_TAG);
    printf("Build time :%s\n", BUILD_TIMESTAMP);
  } else if (strcmp(argv[1], "-d") == 0 || strcmp(argv[1], "--delete") == 0) {
    if (argc < 3) {
      plog(LOG_ERROR, "missing container name");
      list_installed_containers();
      printf("Usage: %s --del <container_name>\n", argv[0]);
      return -1;
    }

    const char *container_name = argv[2];
    char *path = find_container_path(container_name);

    if (!path) {
      plog(LOG_ERROR, "container '%s' not found", container_name);
      return -1;
    }

    printf("Deleting container: %s\n", container_name);

    int ret = delete_path(path);
    free(path);

    if (ret != 0) {
      fprintf(stderr, "Failed to delete container: %s (error %d)\n",
              container_name, ret);
      return -1;
    }

    printf("Successfully deleted: %s\n", container_name);
  } else

      if (strcmp(argv[1], "-r") == 0 || strcmp(argv[1], "--run") == 0) {
    const char *container_name = argc >= 3 ? argv[2] : NULL;
    run_proot_container(container_name);
  } else {
    show_help(argv[0]);
  }
  return 0;
}

int main(int argc, char *argv[]) {
  config_init();
  prase_args(argc, argv);
  config_cleanup();
  return 0;
}