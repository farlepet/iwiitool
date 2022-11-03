#ifndef ANSI_ESCAPE_H
#define ANSI_ESCAPE_H

typedef enum ansi_color_enum {
    ANSI_COLOR_BLACK = 0,
    ANSI_COLOR_RED,
    ANSI_COLOR_GREEN,
    ANSI_COLOR_YELLOW,
    ANSI_COLOR_BLUE,
    ANSI_COLOR_MAGENTA,
    ANSI_COLOR_CYAN,
    ANSI_COLOR_WHITE,
    ANSI_COLOR_MAX
} ansi_color_e;

typedef enum ansi_sgr_enum {
    ANSI_SGR_RESET                    =  0,
    ANSI_SGR_BOLD                     =  1,
    ANSI_SGR_FAINT                    =  2,
    ANSI_SGR_ITALIC                   =  3,
    ANSI_SGR_UNDERLINE                =  4,
    ANSI_SGR_SLOWBLINK                =  5,
    ANSI_SGR_RAPIDBLINK               =  6,
    ANSI_SGR_INVERT                   =  7,
    ANSI_SGR_CONCEAL                  =  8,
    ANSI_SGR_STRIKETHROUGH            =  9,
    ANSI_SGR_FONT_PRIMARY             = 10,
    ANSI_SGR_FONT_START               = 11,
    ANSI_SGR_FONT_END                 = 19,
    ANSI_SGR_FRAKTUR                  = 20,
    ANSI_SGR_DOUBLE_UNDERLINE         = 21,
    ANSI_SGR_NORMAL_INTENSITY         = 22,
    ANSI_SGR_NO_ITALIC                = 23,
    ANSI_SGR_NO_UNDERLINE             = 24,
    ANSI_SGR_NO_BLINKING              = 25,
    ANSI_SGR_PROPORTIONAL_SPACING     = 26,
    ANSI_SGR_NO_INVERT                = 27,
    ANSI_SGR_NO_CONCEALED             = 28,
    ANSI_SGR_NO_STRIKETHROUGH         = 29,
    ANSI_SGR_FOREGROUND_START         = 30,
    ANSI_SGR_FOREGROUND_END           = 37,
    ANSI_SGR_FOREGROUND               = 38,
    ANSI_SGR_FOREGROUND_DEFAULT       = 39,
    ANSI_SGR_BACKGROUND_START         = 40,
    ANSI_SGR_BACKGROUND_END           = 47,
    ANSI_SGR_BACKGROUND               = 48,
    ANSI_SGR_BACKGROUND_DEFAULT       = 49,
    ANSI_SGR_NO_PROPORTIONAL_SPACING  = 50,
    ANSI_SGR_FRAME                    = 51,
    ANSI_SGR_ENCIRCLE                 = 52,
    ANSI_SGR_OVERLINE                 = 53,
    ANSI_SGR_NO_FRAME_ENCIRCLE        = 54,
    ANSI_SGR_NO_OVERLINE              = 55,
    ANSI_SGR_SUPERSCRIPT              = 73,
    ANSI_SGR_SUBSCRIPT                = 74,
    ANSI_SGR_NO_SUPERSCRIPT_SUBSCRIPT = 75,
} ansi_sgr_e;

#endif

