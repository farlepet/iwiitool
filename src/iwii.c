#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "ansi_escape.h"
#include "iwii.h"

int iwii_serial_init(int fd, iwii_flow_e flow, unsigned baud) {
    speed_t speed;
    if(baud == 300) {
        speed = B300;
    } else if(baud == 1200) {
        speed = B1200;
    } else if(baud == 2400) {
        speed = B2400;
    } else if(baud == 9600) {
        speed = B9600;
    } else {
        return -1;
    }

    struct termios tty;

    if(tcgetattr(fd, &tty)) {
        if(errno == ENOTTY) {
            /* We are writing to a file */
            return 0;
        }
        fprintf(stderr, "tcgetattr: %s\n", strerror(errno));
        return -1;
    }

    /* No parity */
    tty.c_cflag &= ~PARENB;
    /* 1 stop bit */
    tty.c_cflag &= ~CSTOPB;
    /* 8-bit */
    tty.c_cflag |= CS8;

    switch(flow) {
        case IWII_FLOW_NONE:
            tty.c_iflag &= ~(IXON | IXOFF);
            tty.c_cflag &= ~CRTSCTS;
            break;
        case IWII_FLOW_XONXOFF:
            tty.c_iflag |=  IXON | IXOFF;
            tty.c_cflag &= ~CRTSCTS;
            break;
        case IWII_FLOW_RTSCTS:
            tty.c_iflag &= ~(IXON | IXOFF);
            tty.c_cflag |=  CRTSCTS;
            break;
        default:
            return -1;

    }

    /* Disable canonical mode */
    tty.c_lflag &= ~ICANON;

    /* Disable unwanted character conversions */
    tty.c_oflag &= ~OPOST;
    tty.c_oflag &= ~ONLCR;

    cfsetispeed(&tty, speed);
    cfsetospeed(&tty, speed);

    if(tcsetattr(fd, TCSANOW, &tty)) {
        fprintf(stderr, "tcsetattr: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

static const char iwii_font[] = {
    [IWII_FONT_EXTENDED]           = 'n',
    [IWII_FONT_PICA]               = 'N',
    [IWII_FONT_ELITE]              = 'E',
    [IWII_FONT_SEMICONDENSED]      = 'e',
    [IWII_FONT_CONDENSED]          = 'q',
    [IWII_FONT_ULTRACONDENSED]     = 'Q',
    [IWII_FONT_PROPORTIONAL_PICA]  = 'p',
    [IWII_FONT_PROPORTIONAL_ELITE] = 'P',
    [IWII_FONT_CUSTOM]             = '\'',
};

int iwii_set_font(int fd, unsigned font) {
    if(font >= IWII_FONT_MAX) {
        return -1;
    }

    char cmd[] = { '\033', iwii_font[font] };
    write(fd, &cmd, sizeof(cmd));

    return 0;
}

static const char iwii_quality[] = {
    [IWII_QUAL_DRAFT]             = '1',
    [IWII_QUAL_STANDARD]          = '0',
    [IWII_QUAL_NEARLETTERQUALITY] = '2'
};

int iwii_set_quality(int fd, unsigned quality) {
    if(quality >= IWII_QUAL_MAX) {
        return -1;
    }

    char cmd[] = { '\033', 'a', iwii_quality[quality] };
    write(fd, &cmd, sizeof(cmd));

    return 0;
}

static const char iwii_color[] = {
    [ANSI_COLOR_BLACK]   = '0',
    [ANSI_COLOR_RED]     = '2',
    [ANSI_COLOR_GREEN]   = '5',
    [ANSI_COLOR_YELLOW]  = '1',
    [ANSI_COLOR_BLUE]    = '3',
    [ANSI_COLOR_MAGENTA] = '6',
    [ANSI_COLOR_CYAN]    = '4', /* Cyan does not exist, mapping to orange instead */
    [ANSI_COLOR_WHITE]   = '0'  /* White would be no printing at all, mapping to black */
};

int iwii_set_color(int fd, unsigned color) {
    if(color >= IWII_COLOR_MAX) {
        return -1;
    }

    dprintf(fd, "\033K%u", color);

    return 0;
}

int iwii_set_ansicolor(int fd, unsigned color) {
    if(color >= ANSI_COLOR_MAX) {
        return -1;
    }

    char cmd[] = { '\033', 'K', iwii_color[color] };
    write(fd, &cmd, sizeof(cmd));

    return 0;
}

/** Max tab positions for each font. Assuming minimum for custom fonts to be safe */
static const uint8_t tab_max[IWII_FONT_MAX] = { 72, 80, 96, 107, 120, 136, 72, 82, 72 };

int iwii_set_tabs(int fd, unsigned tab_size, unsigned font) {
    if(font > IWII_FONT_MAX) {
        return -1;
    }

    /* Start writing tab stops */
    write(fd, "\033(", 2);
    /* Get number of tabs for selected spacing */
    unsigned n = (tab_max[font] / tab_size) - 1;
    if(n > 32) {
        n = 32;
    }

    for(unsigned i = 1; i <= n; i++) {
        dprintf(fd, "%03u", tab_size * i);
        if(i < n) {
            write(fd, ",", 1);
        }
    }

    /* Stop writing tab stops */
    write(fd, ".", 1);

    return 0;
}

int iwii_set_lpi(int fd, unsigned lpi) {
    if((lpi != 6) && (lpi != 8)) {
        return -1;
    }

    char cmd[] = { '\033', (lpi == 6) ? 'A' : 'B' };
    write(fd, &cmd, sizeof(cmd));

    return 0;
}

int iwii_set_line_spacing(int fd, unsigned line_spacing) {
    if((line_spacing < 1) ||
       (line_spacing > 99)) {
        return -1;
    }

    dprintf(fd, "\033T%02u", line_spacing);

    return 0;
}




int iwii_set_left_margin(int fd, unsigned left_margin) {
    if(left_margin > 300) {
        return -1;
    }

    dprintf(fd, "\033L%03u", left_margin);

    return 0;
}
int iwii_set_pagelen(int fd, unsigned pagelen) {
    if((pagelen < 1) ||
       (pagelen > 9999)) {
        return -1;
    }

    dprintf(fd, "\033H%04u", pagelen);

    return 0;
}

int iwii_set_prop_spacing(int fd, unsigned prop_spacing) {
    if(prop_spacing > 9) {
        return -1;
    }

    dprintf(fd, "\033s%u", prop_spacing);

    return 0;
}

int iwii_move_up_lines(int fd, unsigned lines) {
    /* Reverse line feed */
    write(fd, "\er", 2);
    while(lines--) {
        write(fd, "\n", 1);
    }
    /* Forward line feed */
    write(fd, "\ef", 2);

    return 0;
}

