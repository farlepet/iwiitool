#ifndef IWII_H
#define IWII_H

typedef enum iwii_font_enum {
    IWII_FONT_EXTENDED = 0,
    IWII_FONT_PICA,
    IWII_FONT_ELITE,
    IWII_FONT_SEMICONDENSED,
    IWII_FONT_CONDENSED,
    IWII_FONT_ULTRACONDENSED,
    IWII_FONT_PROPORTIONAL_PICA,
    IWII_FONT_PROPORTIONAL_ELITE,
    IWII_FONT_CUSTOM,
    IWII_FONT_MAX
} iwii_font_e;

typedef enum iwii_quality_enum {
    IWII_QUAL_DRAFT = 0,
    IWII_QUAL_STANDARD,
    IWII_QUAL_NEARLETTERQUALITY,
    IWII_QUAL_MAX
} iwii_quality_e;

typedef enum iwii_color_enum {
    IWII_COLOR_BLACK  = 0,
    IWII_COLOR_YELLOW = 1,
    IWII_COLOR_RED    = 2,
    IWII_COLOR_BLUE   = 3,
    IWII_COLOR_ORANGE = 4, /* Yellow + Red */
    IWII_COLOR_GREEN  = 5, /* Yellow + Blue */
    IWII_COLOR_PURPLE = 6, /* Red + Blue */
    IWII_COLOR_MAX    = 7
} iwii_color_e;

typedef enum iwii_flow_enum {
    IWII_FLOW_NONE = 0,
    IWII_FLOW_XONXOFF,
    IWII_FLOW_RTSCTS
} iwii_flow_e;

int iwii_serial_init(int fd, iwii_flow_e flow, unsigned baud);

/**
 * @brief Set current font
 *
 * @param fd File descriptor to which to write escape codes
 * @param font Font to use, @see iwii_font_e
 * @return 0 on success, else < 0
 */
int iwii_set_font(int fd, unsigned font);

int iwii_set_quality(int fd, unsigned quality);

int iwii_set_color(int fd, unsigned color);

int iwii_set_ansicolor(int fd, unsigned color);

int iwii_set_tabs(int fd, unsigned tab_size, unsigned font);

int iwii_set_lpi(int fd, unsigned lpi);

int iwii_set_line_spacing(int fd, unsigned line_spacing);


int iwii_set_left_margin(int fd, unsigned left_margin);

int iwii_set_pagelen(int fd, unsigned pagelen);

int iwii_set_prop_spacing(int fd, unsigned prop_spacing);

int iwii_move_up_lines(int fd, unsigned lines);

#endif

