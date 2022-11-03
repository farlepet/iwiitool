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


int iwii_set_color(int fd, unsigned color);

int iwii_set_font(int fd, unsigned font);

int iwii_set_lpi(int fd, unsigned lpi);

int iwii_set_quality(int fd, unsigned quality);

int iwii_set_left_margin(int fd, unsigned left_margin);

int iwii_set_tabs(int fd, unsigned tab_size, unsigned font);

#endif

