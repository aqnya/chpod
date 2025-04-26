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
#include "include/color.h"

void cprintf(const char *color, const char *format, ...) {
  va_list args;

  // 输出颜色代码
  printf("%s", color);

  // 使用va_list处理可变参数
  va_start(args, format);
  vprintf(format, args);
  va_end(args);

  // 重置颜色
  printf("%s", RESET);
}

void cperror(const char *color, const char *prefix) {
  fprintf(stderr, "%s%s:%s%s\n", color, prefix, strerror(errno),
          (errno != 0) ? "" : "\nNo error.");
  printf("%s", RESET);
}
