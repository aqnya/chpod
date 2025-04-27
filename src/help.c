#include <stdio.h>

#define ANSI_BOLD "\033[1m"
#define ANSI_RESET "\033[0m"

#include "chd.h"

#include <sys/stat.h>

static void print_highlighted_option(const char *option) {
  printf(ANSI_BOLD "%s" ANSI_RESET, option);
}

void print_help(const char *pname) {
  printf("Usage: %s [options] program [arg...]\n", pname);
  printf("Options:\n");

  const char *options[][2] = {{"-h, --help   ", "Show this help information."},
                              {"-i, --pull   ", "Install rootfs."},
                              {"-r, --run    ", "run container with proot."}};

  // 修改循环变量类型为size_t
  for (size_t i = 0; i < sizeof(options) / sizeof(options[0]); i++) {
    printf("  ");
    print_highlighted_option(options[i][0]);
    printf(" %s\n", options[i][1]);
  }
}