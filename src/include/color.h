#ifndef COL_H
#define COL_H
typedef enum {
    FG_DEFAULT = 39,
    FG_BLACK = 30,
    FG_RED,
    FG_GREEN,
    FG_YELLOW,
    FG_BLUE,
    FG_MAGENTA,
    FG_CYAN,
    FG_WHITE
} ForegroundColor;

typedef enum {
    BG_DEFAULT = 49,
    BG_BLACK = 40,
    BG_RED,
    BG_GREEN,
    BG_YELLOW,
    BG_BLUE,
    BG_MAGENTA,
    BG_CYAN,
    BG_WHITE
} BackgroundColor;

typedef enum {
    STYLE_RESET = 0,
    STYLE_BOLD = 1,
    STYLE_UNDERLINE = 4,
    STYLE_REVERSED = 7
} TextStyle;
#endif