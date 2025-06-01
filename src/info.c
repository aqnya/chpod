#include <stdio.h>

#include "chd.h"

void show_help(const char *file) {
  printf("Usage: %s [options] program [arg...]\n", file);
  printf("Options:\n");
  printf("  -i, --install Install rootfs.\n");
  printf("  -h, --help    Show help information.\n");
  printf("  -r, --run     Run container with proot.\n");
  printf("  -d, --del     Delete rootfs.\n");
}