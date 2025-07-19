#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define LOG_WARNING 1
#define LOG_ERROR   2

void plog(int level, const char *format, ...) {
  const char *color_code = "";
  const char *reset_code = "\033[0m";
  const char *level_str = "INFO";

  if (level == LOG_ERROR) {
    color_code = "\033[31m"; // 红色
    level_str = "ERROR";
  } else if (level == LOG_WARNING) {
    color_code = "\033[33m"; // 黄色
    level_str = "WARNING";
  }

  printf("%s[%s] ", color_code, level_str);

  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);

  printf("%s\n", reset_code);
}