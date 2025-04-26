/*
 * Copyright (C) 2023-2024  April8th
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "include/chd.h"

#include <limits.h>
#include <sys/stat.h>

void dispatch_arguments(int argc, char *argv[]) {
  // 检查是否有足够的参数
  if (argc < 2) {
    print_help(argv[0]);
    return;
  }

  // 检查选项类型
  const char *option = argv[1];
  if (option[0] == '-') {
    // 短选项或长选项
    if (option[1] == '-') {
      // 长选项
      if (strcmp(option + 2, "install") == 0) {
        if (argc >= 4) {
          pull(argv[2], argv[3]);
        } else {
          cprintf(RED, "Error: insufficient arguments for --install\n");
          print_help(argv[0]);
        }
      } else if (strcmp(option + 2, "help") == 0) {
        print_help(argv[0]);
      } else if (strcmp(option + 2, "run") == 0) {
        run_proot_container(argv[2]);
      } else if (strcmp(option + 2, "test") == 0) {
        // 测试选项的处理
      } else {
        cprintf(RED, "Error: unknown option \"%s\"\n", option);
        print_help(argv[0]);
      }
    } else {
      // 短选项
      switch (option[1]) {
      case 'i': // -i or --install
        if (argc >= 4) {
          pull(argv[2], argv[3]);
        } else {
          cperror(RED, "Error: insufficient arguments for -i\n");
          print_help(argv[0]);
        }
        break;
      case 'h': // -h or --help
        print_help(argv[0]);
        break;
      case 'r':
        run_proot_container(argv[2]);
        break;
      case 't': // -t or --test
        // 测试选项的处理
        break;
      default:
        cprintf(RED, "Error: unknown option \"%s\"\n", option);
        print_help(argv[0]);
      }
    }
  } else {
    cprintf(RED, "Error: unknown option \"%s\"\n", option);
    print_help(argv[0]);
  }
}

int main(int argc, char *argv[]) {
  // 检查环境变量
  config_init();

  unsetenv("LD_PRELOAD");
#ifdef DEBUG
  printf("HOME: %s\n", getenv("HOME"));
  printf("PREFIX: %s\n", getenv("PREFIX"));
  printf("\nPid:[%d].", getpid());
  printf("\nUid:[%d].\n", getuid());
#endif
  dispatch_arguments(argc, argv);
  config_cleanup();
  return 0;
}
