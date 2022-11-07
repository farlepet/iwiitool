#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "iwii.h"
#include "iwii_gfx.h"

typedef struct {
    uint8_t h_dpi; /**< Horizontal dots per inch */
    uint8_t v_dpi; /**< Vertical dots per inch */
} iwii_gfx_state_t;

static iwii_gfx_state_t _gfx_state;

int iwii_gfx_init(int fd, unsigned h_dpi, unsigned v_dpi) {
    memset(&_gfx_state, 0, sizeof(_gfx_state));


    
    /* Horizontal DPI is determined by the currently selected font */
    iwii_font_e font;
    switch(h_dpi) {
        case 72:
            font = IWII_FONT_EXTENDED;
            break;
        case 80:
            font = IWII_FONT_PICA;
            break;
        case 96:
            font = IWII_FONT_ELITE;
            break;
        case 107:
            font = IWII_FONT_SEMICONDENSED;
            break;
        case 120:
            font = IWII_FONT_CONDENSED;
            break;
        case 136:
            font = IWII_FONT_ULTRACONDENSED;
            break;
        case 144:
            font = IWII_FONT_PICA;
            break;
        case 160:
            font = IWII_FONT_EXTENDED;
            break;
        default:
            return -1;
    }

    /* Dots are spaced 1/72 inches apart, 144dpi is acheived be doing stepping
     * done 144th of an inch. */
    if((v_dpi != 72) && (v_dpi != 144)) {
        return -1;
    }

    if(iwii_set_font(fd, font) ||
       iwii_set_line_spacing(fd, 16)) {
        return -1;
    }

    _gfx_state.h_dpi = h_dpi;
    _gfx_state.v_dpi = v_dpi;

    return 0;
}

/**
 * @brief Print a single line of gfx data, in a single (pre-set) color
 *
 * @param fd File descriptor to write to
 * @param data Buffer where each byte represents one column of 8 dots
 * @param len Length of buffer/line
 */
static int iwii_gfx_print_line(int fd, const uint8_t *data, unsigned len) {
    if(len > 9999) {
        return -1;
    }

    dprintf(fd, "\033G%04u", len);
    if(write(fd, data, len) < 0) {
        return -1;
    }

    return 0;
}

/**
 * @brief Tests if the given color required using the current ribbon
 * 
 * @param ribbon Current ribbon
 * @param color  Color to test
 */
static inline int _test_color(uint8_t ribbon, uint8_t color) {
    switch(ribbon) {
        case 0: /* Yellow */
            return (color == IWII_COLOR_YELLOW) ||
                   (color == IWII_COLOR_ORANGE) ||
                   (color == IWII_COLOR_GREEN);
        case 1: /* Red */
            return (color == IWII_COLOR_RED) ||
                   (color == IWII_COLOR_ORANGE) ||
                   (color == IWII_COLOR_PURPLE);
        case 2: /* Blue */
            return (color == IWII_COLOR_BLUE) ||
                   (color == IWII_COLOR_GREEN) ||
                   (color == IWII_COLOR_PURPLE);
        case 3: /* Black */
            return color == IWII_COLOR_BLACK;
    }

    return 0;
}

/**
 * @brief Print a single line of gfx data, with all necessary colors
 *
 * @note This method is not terribly efficient, but it's good enough for now.
 *
 * @param fd File descriptor to write to
 * @param data Buffer containing indexed color data, each byte containing one pixel
 * @param width Width of image, in pixels
 * @param rows Numer of rows to print, should be 8 an all lines but the final line
 */
static int iwii_gfx_print_line_color(int fd, const uint8_t *data, unsigned width, unsigned rows) {
    uint8_t *line = malloc(width);
    if(line == NULL) {
        return -1;
    }

    for(uint8_t i = 0; i < 4; i++) {
        /* At most a 4-pass process. Starting with yellow following recommendation
         * from manual to prevent staining yellow portion of ribbon.1
         * 0: Yellow
         * 1: Red
         * 2: Blue 
         * 3: Black*/
        int wr = 0;

        for(unsigned j = 0; j < width; j++) {
            uint8_t col = 0;
            for(int k = rows - 1; k >= 0; k--) {
                col = (col << 1) | (_test_color(i, data[(k * width) + j]) ? 1 : 0);
            }
            if(col) {
                wr = 1;
            }
            line[j] = col;
        }

        /* Only write if color is used in line */
        if(wr) {
            iwii_set_color(fd, (i == 0) ? IWII_COLOR_YELLOW :
                               (i == 1) ? IWII_COLOR_RED    :
                               (i == 2) ? IWII_COLOR_BLUE   : IWII_COLOR_BLACK);

            /* Start carriage to 0
             * @todo Allow image to be positioned anywhere */
            dprintf(fd, "\r\033F0000");
            
            iwii_gfx_print_line(fd, line, width);
        }
    }

    return 0;
}



int iwii_gfx_print_image(int fd, const uint8_t *data, unsigned width, unsigned height) {
#if 0
    /* Test conversion when using some GIMP-generated C arrays. */
    for(unsigned i = 0; i < (width * height); i++) {
        switch(data[i]) {
            case 0:
                data[i] = IWII_COLOR_MAX;
                break;
            case 1:
                data[i] = IWII_COLOR_BLACK;
                break;
            case 2:
                data[i] = IWII_COLOR_BLUE;
                break;
            case 3:
                data[i] = IWII_COLOR_PURPLE;
                break;
            case 4:
                data[i] = IWII_COLOR_GREEN;
                break;
            case 5:
                data[i] = IWII_COLOR_RED;
                break;
            case 6:
                data[i] = IWII_COLOR_YELLOW;
                break;
            case 7:
                data[i] = IWII_COLOR_ORANGE;
                break;
        }
    }
#endif

    for(unsigned i = 0; i < height; i += 8) {
        unsigned rows = 8;
        if((height - i) < rows) {
            rows = height - i;
        }

        iwii_gfx_print_line_color(fd, &data[i * width], width, rows);
        write(fd, "\r\n", 2);
    }

    return 0;
}


int iwii_gfx_test(int fd) {
    if(iwii_gfx_init(fd, 72, 72)) {
        return -1;
    }

    const uint8_t img[] = { 0, 0, 1, 1, 2, 2, 3, 3,
                            0, 0, 1, 1, 2, 2, 3, 3,
                            1, 1, 2, 2, 3, 3, 4, 4,
                            1, 1, 2, 2, 3, 3, 4, 4,
                            2, 2, 3, 3, 4, 4, 5, 5,
                            2, 2, 3, 3, 4, 4, 5, 5,
                            3, 3, 4, 4, 5, 5, 6, 6,
                            3, 3, 4, 4, 5, 5, 6, 6 };

    if(iwii_gfx_print_image(fd, img, 8, 8)) {
        return -1;
    }

    return 0;
}
