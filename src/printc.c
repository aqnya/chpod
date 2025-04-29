#include <stdio.h>
#include <stdarg.h>
#include "color.h"

void printc(ForegroundColor fg, BackgroundColor bg, TextStyle style, const char *format, ...) {
    va_list args;

    printf("\033[%d;%d;%dm", style, fg, bg);

    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    printf("\033[0m");
}