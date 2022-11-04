#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "iwii.h"
#include "ansi_escape.h"

typedef struct opts_struct {
/* I/O Config */
    int      fd_in;     /**< Input file descriptor */
    int      fd_out;    /**< Output file descriptor */
    uint8_t  flow;      /**< Flow control method to use */
#define OPT_FLOW_NONE          (0)
#define OPT_FLOW_XONXOFF       (1)
#define OPT_FLOW_RTSCTS        (2)
    speed_t  baud;      /**< Baud rate to use */

/* Configuration */
    uint8_t  verbose;   /**< Program verbosity level */
    uint32_t setflags;  /**< Set of flags of which settings to apply */
#define OPT_SETFLAG_FONT            (1UL <<  0)
#define OPT_SETFLAG_COLOR           (1UL <<  1)
#define OPT_SETFLAG_QUALITY         (1UL <<  2)
#define OPT_SETFLAG_TAB             (1UL <<  3)
#define OPT_SETFLAG_LINESPERINCH    (1UL <<  4)
#define OPT_SETFLAG_LINESPACING     (1UL <<  5)
#define OPT_SETFLAG_LEFTMARGIN      (1UL <<  6)
#define OPT_SETFLAG_PAGELEN         (1UL <<  7)
#define OPT_SETFLAG_SKIPPERFORATION (1UL <<  8)
#define OPT_SETFLAG_UNIDIRECTIONAL  (1UL <<  9)
#define OPT_SETFLAG_AUTOLINEFEED    (1UL << 10)
#define OPT_SETFLAG_SLASHEDZERO     (1UL << 11)
#define OPT_SETFLAG_DOUBLEWIDTH     (1UL << 12)
#define OPT_SETFLAG_PROPSPACING     (1UL << 13)
    uint8_t  font;        /**< Default font to use */
    uint8_t  quality;     /**< Default print quality */
    uint8_t  color;       /**< Default color to use */
    uint8_t  tab;         /**< Tab spacing */
    uint8_t  lpi;         /**< Default lines per inch */
    uint8_t  linespacing; /**< Line spacing */
    
    uint8_t  leftmargin;  /**< Left margin */
    uint16_t pagelen;     /**< Page length */
    uint8_t  cfgflags;    /**< Config en/disable flags */
#define OPT_CFGFLAG_SKIPPERFORATION (1UL << 0) /**< Skip page perforation */
#define OPT_CFGFLAG_UNIDIRECTIONAL  (1UL << 1) /**< Print in only one direction */
#define OPT_CFGFLAG_AUTOLINEFEED    (1UL << 2) /**< Automatically Insert line feed at end of line */
#define OPT_CFGFLAG_SLASHEDZERO     (1UL << 3) /**< Use slashed zero */
#define OPT_CFGFLAG_DOUBLEWIDTH     (1UL << 4) /**< Double-width characters */
    uint8_t  propspacing; /**< Proportional character spacing */

/* State variables */
    uint32_t flags;     /**< Configuration flags, and default state */
#define OPT_FLAG_STRIKETHROUGH      (1UL <<  0) /**< Strikethrough enabled */
#define OPT_FLAG_CONCEAL            (1UL <<  1) /**< Conceal enabled */
#define OPT_FLAG_IDENTIFY           (1UL << 29) /**< Request identity from printer */
#define OPT_FLAG_NOSETUP            (1UL << 30) /**< Do not configure printer at startup */
#define OPT_FLAG_ENABLECOLOR        (1UL << 31) /**< Enable color escape codes */
    uint8_t  font_curr; /**< Font currently in use */
    uint8_t  font_save; /**< Saved font when switching to proportional */
} opts_t;

static opts_t opts = {
/* I/O Config */
    .fd_in     = STDIN_FILENO,
    .fd_out    = STDOUT_FILENO,
    .flow      = OPT_FLOW_XONXOFF,
    .baud      = B9600,

/* Config */
    .verbose     = 0,
    .setflags    = OPT_SETFLAG_FONT | OPT_SETFLAG_TAB,
    .font        = IWII_FONT_ELITE,
    .quality     = IWII_QUAL_DRAFT,
    .color       = ANSI_COLOR_BLACK,
    .tab         = 8,
    .lpi         = 0,
    .linespacing = 0,
    .leftmargin  = 0,
    .pagelen     = 0,
    .cfgflags    = 0,

/* State variables */
#define OPT_FLAG_NOSETUP            (1UL << 30) /**< Do not configure printer at startup */
    .flags     = 0,
    .font_curr = IWII_FONT_ELITE,
    .font_save = 0xff
};

static int _setup_serial(void);
static int _identify(void);
static int _apply_config(void);
static int _handle_char(char c);
static int _handle_args(int argc, char **const argv);

#define BUFF_SZ 64

int main(int argc, char **argv) {
    (void)opts;

    if(_handle_args(argc, argv)) {
        return -1;
    }

    if(_setup_serial()) {
        close(opts.fd_in);
        close(opts.fd_out);
        return -1;
    }

    if(opts.flags & OPT_FLAG_IDENTIFY) {
        if(_identify()) {
            close(opts.fd_in);
            close(opts.fd_out);
            return -1;
        }
        close(opts.fd_in);
        close(opts.fd_out);
        return 0;
    }

    if(!(opts.flags & OPT_FLAG_NOSETUP)) {
        if(_apply_config()) {
            close(opts.fd_in);
            close(opts.fd_out);
            return -1;
        }
    }

    char *buff = malloc(BUFF_SZ);
    if(buff == NULL) {
        fprintf(stderr, "Could not allocate space for input buffer: %s\n", strerror(errno));
        close(opts.fd_in);
        close(opts.fd_out);
        return -1;
    }

    while(1) {
        ssize_t rd = read(opts.fd_in, buff, BUFF_SZ);
        if(rd > 0) {
            for(ssize_t i = 0; i < rd; i++) {
                _handle_char(buff[i]);
            }
        } else if(rd < 0) {
            fprintf(stderr, "Error reading from input: %s\n", strerror(errno));
            goto main_fail;
        } else {
            /* EOF */
            break;
        }
    }

    free(buff);
    close(opts.fd_in);
    close(opts.fd_out);

    return 0;

main_fail:
    free(buff);
    close(opts.fd_in);
    close(opts.fd_out);

    return -1;
}

static int _setup_serial(void) {
    struct termios tty;

    if(tcgetattr(opts.fd_out, &tty)) {
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

    if(opts.flow == OPT_FLOW_XONXOFF) {
        tty.c_iflag |=  IXON | IXOFF;
        tty.c_cflag &= ~CRTSCTS;
    } else if(opts.flow == OPT_FLOW_RTSCTS) {
        tty.c_iflag &= ~(IXON | IXOFF);
        tty.c_cflag |=  CRTSCTS;
    } else {
        tty.c_iflag &= ~(IXON | IXOFF);
        tty.c_cflag &= ~CRTSCTS;
    }

    /* Disable canonical mode */
    tty.c_lflag &= ~ICANON;

    /* Disable unwanted character conversions */
    tty.c_oflag &= ~OPOST;
    tty.c_oflag &= ~ONLCR;

    cfsetispeed(&tty, opts.baud);
    cfsetospeed(&tty, opts.baud);

    if(tcsetattr(opts.fd_out, TCSANOW, &tty)) {
        fprintf(stderr, "tcsetattr: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

static int _identify(void) {
    write(opts.fd_out, "\033?", 2);
    
    char resp[8];
    int i = 0;
    memset(resp, 0, sizeof(resp));

    while (read(opts.fd_out, &resp[i], sizeof(resp)-1)) {
        if(i >= 8) {
            return -1;
        }
        if(resp[i] == '\r') {
            resp[i] = 0;
            printf("Identity response: %s\n", resp);
            break;
        }
        i++;
    }

    return 0;
}

static int _apply_config(void) {
    if(opts.setflags & OPT_SETFLAG_FONT) {
        iwii_set_font(opts.fd_out, opts.font);
    }
    if(opts.setflags & OPT_SETFLAG_QUALITY) {
        iwii_set_quality(opts.fd_out, opts.quality);
    }
    if((opts.flags & OPT_FLAG_ENABLECOLOR) &&
       (opts.setflags & OPT_SETFLAG_COLOR)) {
        iwii_set_color(opts.fd_out, opts.color);
    }
    if(opts.setflags & OPT_SETFLAG_TAB) {
        iwii_set_tabs(opts.fd_out, opts.tab, opts.font);
    }
    if(opts.setflags & OPT_SETFLAG_LINESPERINCH) {
        iwii_set_lpi(opts.fd_out, opts.lpi);
    }
    if(opts.setflags & OPT_SETFLAG_LINESPACING) {
        iwii_set_line_spacing(opts.fd_out, opts.linespacing);
    }
    
    if(opts.setflags & OPT_SETFLAG_LEFTMARGIN) {
        iwii_set_left_margin(opts.fd_out, opts.leftmargin);
    }
    if(opts.setflags & OPT_SETFLAG_PAGELEN) {
        iwii_set_pagelen(opts.fd_out, opts.pagelen);
    }
    if(opts.setflags & OPT_SETFLAG_PROPSPACING) {
        iwii_set_prop_spacing(opts.fd_out, opts.propspacing);
    }
    
    if(opts.setflags & OPT_SETFLAG_SKIPPERFORATION) {
        write(opts.fd_out, (opts.cfgflags & OPT_CFGFLAG_SKIPPERFORATION) ? "\033D\x00\x04"
                                                                         : "\033Z\x00\x04", 4);
    }
    if(opts.setflags & OPT_SETFLAG_UNIDIRECTIONAL) {
        write(opts.fd_out, (opts.cfgflags & OPT_CFGFLAG_UNIDIRECTIONAL) ? "\033>"
                                                                        : "\033<", 2);
    }
    if(opts.setflags & OPT_SETFLAG_AUTOLINEFEED) {
        write(opts.fd_out, (opts.cfgflags & OPT_CFGFLAG_AUTOLINEFEED) ? "\033D \x00"
                                                                      : "\033Z \x00", 4);
    }
    if(opts.setflags & OPT_SETFLAG_SLASHEDZERO) {
        write(opts.fd_out, (opts.cfgflags & OPT_CFGFLAG_SLASHEDZERO) ? "\033D\x00\x01"
                                                                     : "\033Z\x00\x01", 4);
    }
    if(opts.setflags & OPT_SETFLAG_DOUBLEWIDTH) {
        write(opts.fd_out, (opts.cfgflags & OPT_CFGFLAG_DOUBLEWIDTH) ? "\x0e"
                                                                     : "\x0f", 1);
    }

    return 0;
}

static void _ansi_reset(void) {
    opts.flags &= ~(OPT_FLAG_STRIKETHROUGH | OPT_FLAG_CONCEAL);
    /* No bold, underline, sub/superscript, or italic */
    const char *reset = "\033\"\033Y\033z\033W";
    write(opts.fd_out, reset, strlen(reset));

    iwii_set_font(opts.fd_out, opts.font);

    if(opts.flags & OPT_FLAG_ENABLECOLOR) {
        iwii_set_color(opts.fd_out, opts.color);
    }
}

/* Table of single-character ESC code conversion */
static const char _ansi_iwii_codes[] = {
    [ANSI_SGR_BOLD]                     = '!',
    [ANSI_SGR_NORMAL_INTENSITY]         = '"',
    [ANSI_SGR_UNDERLINE]                = 'X',
    [ANSI_SGR_NO_UNDERLINE]             = 'Y',
    [ANSI_SGR_SUPERSCRIPT]              = 'x',
    [ANSI_SGR_SUBSCRIPT]                = 'y',
    [ANSI_SGR_NO_SUPERSCRIPT_SUBSCRIPT] = 'z',
    /* Italic = half hight, at least for now. */
    [ANSI_SGR_ITALIC]                   = 'w',
    [ANSI_SGR_NO_ITALIC]                = 'W',
};

static int _handle_char(char c) {
    /* @todo It would be more efficient to buffer up writes and send them in chunks. */
#define ANSIBUFF_SZ 8
    static char     ansi_buf[ANSIBUFF_SZ];
    static unsigned ansi_pos = 0;

    if(ansi_pos) {
        ansi_buf[ansi_pos++] = c;
        if(ansi_pos >= ANSIBUFF_SZ) {
            /* Buffer overflow */
            goto ansi_error;
        } else if(ansi_pos == 2) {
            if(c != '[') {
                /* Only CSI escape sequences are presently supported */
                goto ansi_error;
            }
        } else if(isalpha(c)) {
            if(c != 'm') {
                /* Only SGR escape sequences are presently supported */
                goto ansi_error;
            }
            unsigned sgr = strtoul(&ansi_buf[2], NULL, 10);
            if((sgr < sizeof(_ansi_iwii_codes)) &&
               (_ansi_iwii_codes[sgr] != 0)) {
                dprintf(opts.fd_out, "\033%c", _ansi_iwii_codes[sgr]);
            } else {
                if((sgr >= ANSI_SGR_FONT_START) &&
                   (sgr < (ANSI_SGR_FONT_START + IWII_FONT_MAX))) {
                    opts.font_curr = sgr - ANSI_SGR_FONT_START;
                    iwii_set_font(opts.fd_out, opts.font_curr);
                } else if((sgr >= ANSI_SGR_FOREGROUND_START) &&
                          (sgr <= ANSI_SGR_FOREGROUND_END)) {
                    if(opts.flags & OPT_FLAG_ENABLECOLOR) {
                        iwii_set_color(opts.fd_out, sgr - ANSI_SGR_FOREGROUND_START);
                    }
                } else {
                    switch(sgr) {
                        case ANSI_SGR_RESET:
                            _ansi_reset();
                            break;
                        case ANSI_SGR_STRIKETHROUGH:
                            opts.flags |= OPT_FLAG_STRIKETHROUGH;
                            break;
                        case ANSI_SGR_NO_STRIKETHROUGH:
                            opts.flags &= ~OPT_FLAG_STRIKETHROUGH;
                            break;
                        case ANSI_SGR_CONCEAL:
                            opts.flags |= OPT_FLAG_CONCEAL;
                            break;
                        case ANSI_SGR_NO_CONCEAL:
                            opts.flags &= ~OPT_FLAG_CONCEAL;
                            break;
                        case ANSI_SGR_FONT_PRIMARY:
                            opts.font_curr = opts.font;
                            iwii_set_font(opts.fd_out, opts.font_curr);
                            break;
                        case ANSI_SGR_PROPORTIONAL_SPACING:
                            if((opts.font_curr == IWII_FONT_PROPORTIONAL_PICA) ||
                               (opts.font_curr == IWII_FONT_PROPORTIONAL_ELITE)) {
                                /* Already proportional */
                                break;
                            }
                            opts.font_save = opts.font_curr;
                            opts.font_curr = (opts.font_save >= IWII_FONT_ELITE) ? IWII_FONT_PROPORTIONAL_ELITE :
                                                                                   IWII_FONT_PROPORTIONAL_PICA;
                            iwii_set_font(opts.fd_out, opts.font_curr);
                            break;
                        case ANSI_SGR_NO_PROPORTIONAL_SPACING:
                            if((opts.font_curr != IWII_FONT_PROPORTIONAL_PICA) &&
                               (opts.font_curr != IWII_FONT_PROPORTIONAL_ELITE)) {
                                /* Already fixed-width (assuming custom font is fixed) */
                                break;
                            }
                            if(opts.font_save != 0xff) {
                                opts.font_curr = opts.font_save;
                            } else {
                                opts.font_curr = (opts.font_curr == IWII_FONT_PROPORTIONAL_ELITE) ? IWII_FONT_ELITE :
                                                                                                    IWII_FONT_PICA;
                            }
                            iwii_set_font(opts.fd_out, opts.font_curr);
                            break;
                        default:
                            /* Unsupported SGR */
                            goto ansi_error;
                    }
                }
            }
            ansi_pos = 0;
        }
    } else if(c == '\033') {
        ansi_buf[0] = c;
        ansi_pos = 1;
    } else {
        if(opts.flags & OPT_FLAG_CONCEAL) {
            c = ' ';
        } 
        write(opts.fd_out, &c, 1);
        if(opts.flags & OPT_FLAG_STRIKETHROUGH) {
            /* @note This is highly inefficient, the print head has to move
             * back and fourth rapidly to cover each character. A potential
             * optimization would be to keep track of strokethrough works, and
             * only striking through a line at a time. */
            const char st[] = { '\b', '-' };
            write(opts.fd_out, &st, 2);
        } 
    }

    return 0;

ansi_error:
    if(opts.verbose >= 1) {
        ansi_buf[ansi_pos] = 0;
        fprintf(stderr, "ansi_error: `%s`\n", &ansi_buf[1]);
    }
    write(opts.fd_out, &ansi_buf[1], ansi_pos-1);
    ansi_pos = 0;
    return -1;
}

static void _help(void) {
    puts("ansi2iwii: Convert ANSI escape codes to Apple ImageWriter II escape codes\n");

    puts("Basic Options:\n"
         "  -i, --input=FILE          Read input from FILE, use `-` for stdin (default)\n"
         "  -o, --output=FILE         Write output to FILE, use `-` for stdout (default)\n"
         "  -b, --baud=RATE           Set baud rate to use when output is set to the printer's serial\n"
         "                            port. Values 300, 1200, 2400, and 9600 (default) are accepted\n"
         "  -F, --flow=MODE           Set flow control mode when using serial as output\n"
         "                              0: None\n"
         "                              1: XON/XOFF (default)\n"
         "                              2: RTS/CTS\n"
         "  -N, --no-setup            Do not configure printer via escape codes on startup\n"
         "\n"
         "Common Format Options:\n"
         "  -f, --font=FONT           Set default font to use:\n"
         "                              0: Extended\n"
         "                              1: Pica\n"
         "                              2: Elite (default)\n"
         "                              3: Semicondensed\n"
         "                              4: Condensed\n"
         "                              5: Ultracondensed\n"
         "                              6: Pica proportional\n"
         "                              7: Elite proportional\n"
         "                              8: Custom\n"
         "  -q, --quality=QUAL        Set print quality to use:\n"
         "                              0: Draft (default)\n"
         "                              1: Standard\n"
         "                              2: Near Letter Quality\n"
         "  -c, --color[=COLOR]       Enable support for color, set default color if supplied.\n"
         "                              0: Black (default)\n"
         "                              1: Red\n"
         "                              2: Green\n"
         "                              3: Yellow\n"
         "                              4: Blue\n"
         "                              5: Purple\n"
         "                              6: Orange\n"
         "  -t, --tab=WIDTH           Set tab width, in characters (default is 8)\n"
         "                            NOTE: Tab positions are relative to the starting font\n"
         "  -l, --lpi=LPI             Set number of lines per inch, 6 (default) or 8\n"
         "  -L, --line-spacing=SPACE  Set spacing between lines, in 144ths of an inch (1-99)\n"
         "\n"
         "Page Settings:\n"
         "  -M, --left-margin=MARGIN     Set left margin in characters\n"
         "  -p, --pagelen=LENGTH         Set page length in 144ths of an inch\n"
         "  -P, --skip-perforation[=EN]  Enable/disable continuous-form perforation skip\n"
         "\n"
         "Misc. Print Settings:\n"
         "  -U, --unidirectional[=EN]    Enable/disable unidirectional printing\n"
         "  -A, --auto-linefeed[=EN]     Enable/disable automatic linefeed at end of line\n"
         "  -Z, --slashed-zero[=EN]      Enable/disable slashed-zeros\n"
         "  -D, --double-width[=EN]      Enable/disable double-width\n"
         "  -S, --prop-spacing=DOTS      Set proportional dot spacing (0-9)\n"
         "\n"
         "Miscellaneous:\n"
         "  -I, --identify            Retrieve printer identification and exit\n"
         "                            NOTE: Must be provided prior to specifying output\n"
         "  -h, --help                Display this help message\n"
         "  -v, --verbose[=LEVEL]     Increase verbosity, can be supplied multiple times, or desired\n"
         "                            verbosity can be directly supplied\n"
        );

    exit(0);
}

static const struct option prog_options[] = {
    /* Basic Options */
    { "input",            required_argument, NULL, 'i' },
    { "output",           required_argument, NULL, 'o' },
    { "baud",             required_argument, NULL, 'b' },
    { "flow",             required_argument, NULL, 'F' },
    { "no-setup",         no_argument,       NULL, 'N' },
    /* Common Format Options */
    { "font",             required_argument, NULL, 'f' },
    { "quality",          required_argument, NULL, 'q' },
    { "color",            optional_argument, NULL, 'c' },
    { "tab",              required_argument, NULL, 't' },
    { "lpi",              required_argument, NULL, 'l' },
    { "line-spacing",     required_argument, NULL, 'L' },
    /* Page Settings */
    { "left-margin",      required_argument, NULL, 'M' },
    { "pagelen",          required_argument, NULL, 'p' },
    { "skip-perforation", optional_argument, NULL, 'P' },
    /* Misc. Print Settings */
    { "unidirectional",   optional_argument, NULL, 'U' },
    { "auto-linefeed",    optional_argument, NULL, 'A' },
    { "slashed-zero",     optional_argument, NULL, 'Z' },
    { "double-width",     optional_argument, NULL, 'D' },
    { "prop-spacing",     required_argument, NULL, 'S' },
    /* Miscellaneous */
    { "identify",         no_argument,       NULL, 'I' },
    { "help",             no_argument,       NULL, 'h' },
    { "verbose",          optional_argument, NULL, 'v' },
    { NULL, 0, NULL, 0 }
};

static int __get_number(const char *arg, unsigned min, unsigned max, const char *msg) {
    if(!isdigit(arg[0])) {
        fprintf(stderr, "%s must be a number between %u and %u!\r\n", msg, min, max);
        return -1;
    }

    unsigned val = strtoul(optarg, NULL, 10);
    if((val < min) || (val > max)) {
        fprintf(stderr, "%s must be a number between %u and %u!\r\n", msg, min, max);
        return -1;
    }
    
    return val;
}

#define _get_number(min, max, msg, var) { \
    int val = __get_number(optarg, min, max, msg); \
    if(val < 0) { \
        return -1; \
    } \
    var = val; \
}

#define _get_optbool(var, flag) { \
    if(optarg) { \
        if(!strcasecmp(optarg, "Y")) { \
            (var) |= (flag); \
        } else if(!strcasecmp(optarg, "N")) { \
            (var) &= ~(flag); \
        } else { \
            fprintf(stderr, "Optional boolean arguments must be Y/y or N/n\n"); \
            return -1; \
        } \
    } else { \
        (var) |= (flag); \
    } \
}


static int _handle_args(int argc, char **const argv) {
    int c;
    while ((c = getopt_long(argc, argv, "i:o:b:F:N"
                                        "f:q:c::t:l:L:"
                                        "M:p:P::"
                                        "U::A::Z::D::S:"
                                        "Ihv::", prog_options, NULL)) >= 0) {
        switch(c) {
            case 'i':
                if(!strcmp(optarg, "-")) {
                    opts.fd_in = STDIN_FILENO;
                } else {
                    opts.fd_in = open(optarg, O_RDONLY);
                    if(opts.fd_in < 0) {
                        fprintf(stderr, "Could not open input `%s`: %s\n", optarg, strerror(errno));
                        return -1;
                    }
                }
                break;
            case 'o':
                if(!strcmp(optarg, "-")) {
                    opts.fd_out = STDOUT_FILENO;
                } else {
                    if(opts.flags & OPT_FLAG_IDENTIFY) {
                        /* @todo Only open file _after_ all options are processed */
                        opts.fd_out = open(optarg, O_RDWR | O_NOCTTY);
                    } else {
                        opts.fd_out = open(optarg, O_WRONLY | O_NOCTTY);
                    }
                    if(opts.fd_out < 0) {
                        fprintf(stderr, "Could not open output `%s`: %s\n", optarg, strerror(errno));
                        return -1;
                    }
                }
                break;
            case 'b': {
                if(!isdigit(optarg[0])) {
                    fprintf(stderr, "Baud rate selection must 300, 1200, 2400, or 9600!\n");
                    return -1;
                }
                unsigned baud = strtoul(optarg, NULL, 10);
                if(baud == 300) {
                    opts.baud = B300;
                } else if(baud == 1200) {
                    opts.baud = B1200;
                } else if(baud == 2400) {
                    opts.baud = B2400;
                } else if(baud == 9600) {
                    opts.baud = B9600;
                } else {
                    fprintf(stderr, "Baud rate selection must 300, 1200, 2400, or 9600!\n");
                    return -1;
                }
            } break;
            case 'F':
                _get_number(0, 2, "Flow control selection", opts.flow);
                break;
            case 'N':
                opts.flags |= OPT_FLAG_NOSETUP;
                break;

            case 'f':
                opts.setflags |= OPT_SETFLAG_FONT;
                _get_number(0, 8, "Font selection", opts.font);
                break;
            case 'q':
                opts.setflags |= OPT_SETFLAG_QUALITY;
                _get_number(0, 2, "Quality selection", opts.quality);
                break;
            case 'c':
                opts.flags |= OPT_FLAG_ENABLECOLOR;
                if(optarg) {
                    opts.setflags |= OPT_SETFLAG_COLOR;
                    _get_number(0, 6, "Color selection", opts.color);
                }
                break;
            case 't':
                opts.setflags |= OPT_SETFLAG_TAB;
                _get_number(2, 32, "Tab spacing", opts.tab);
                break;
            case 'l':
                opts.setflags |= OPT_SETFLAG_LINESPERINCH;
                if(!isdigit(optarg[0])) {
                    fprintf(stderr, "Lines per inch must be either 6 to 8!\n");
                    return -1;
                }
                opts.lpi = strtoul(optarg, NULL, 10);
                if((opts.lpi != 6) && (opts.lpi != 8)) {
                    fprintf(stderr, "Lines per inch must be either 6 or 8!\n");
                    return -1;
                }
                break;
            case 'L':
                opts.setflags |= OPT_SETFLAG_LINESPACING;
                _get_number(1, 99, "Line spacing", opts.linespacing);
                break;
            
            case 'M':
                opts.setflags |= OPT_SETFLAG_LEFTMARGIN;
                /* The maximum technically depends on the selected font */
                _get_number(0, 136, "Left margin", opts.leftmargin);
                break;
            case 'p':
                opts.setflags |= OPT_SETFLAG_PAGELEN;
                _get_number(1, 9999, "Page length", opts.pagelen);
                break;
            case 'P':
                opts.setflags |= OPT_SETFLAG_SKIPPERFORATION;
                _get_optbool(opts.cfgflags, OPT_CFGFLAG_SKIPPERFORATION);
                break;
            
            case 'U':
                opts.setflags |= OPT_SETFLAG_UNIDIRECTIONAL;
                _get_optbool(opts.cfgflags, OPT_CFGFLAG_UNIDIRECTIONAL);
                break;
            case 'A':
                opts.setflags |= OPT_SETFLAG_AUTOLINEFEED;
                _get_optbool(opts.cfgflags, OPT_CFGFLAG_AUTOLINEFEED);
                break;
            case 'Z':
                opts.setflags |= OPT_SETFLAG_SLASHEDZERO;
                _get_optbool(opts.cfgflags, OPT_CFGFLAG_SLASHEDZERO);
                break;
            case 'D':
                opts.setflags |= OPT_SETFLAG_DOUBLEWIDTH;
                _get_optbool(opts.cfgflags, OPT_CFGFLAG_DOUBLEWIDTH);
                break;
            case 'S':
                opts.setflags |= OPT_SETFLAG_FONT;
                _get_number(0, 9, "Proportional spacing", opts.propspacing);
                break;

            case 'I':
                opts.flags |= OPT_FLAG_IDENTIFY;
                break;
            case 'h':
                _help();
                break;
            case 'v':
                if(optarg) {
                    if(!isdigit(optarg[0])) {
                        fprintf(stderr, "Verbosity level must be a number >= 0!\n");
                        return -1;
                    }
                    opts.verbose = strtoul(optarg, NULL, 10);
                } else {
                    opts.verbose++;
                }
                break;

            case '?':
                return -1;
            default:
                fprintf(stderr, "Unhandled argument: %c\n", c);
                return -1;
        }
    }

    return 0;
}

