#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

typedef enum { WARNING, ERROR } LogLevel;

void plog(LogLevel level, const char *format, ...) {
  const char *color_code = "";
  const char *reset_code = "";
  if (level == ERROR) {
    color_code = "\033[31m"; // red
    reset_code = "\033[0m";
  } else if (level == WARNING) {
    color_code = "\033[33m"; // yellow
    reset_code = "\033[0m";
  }

  printf("%s[%s] ", color_code, level == ERROR ? "ERROR" : "WARNING");

  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
  printf("%s", reset_code);

  printf("\n");
}
