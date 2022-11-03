#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "iwii.h"
#include "ansi_escape.h"

typedef struct opts_struct {
    int      fd_in;   /**< Input file descriptor */
    int      fd_out;  /**< Output file descriptor */
    uint32_t flags;   /**< Configuration flags, and default state */
#define OPT_FLAG_VERBOSE     (1UL << 30)
#define OPT_FLAG_ENABLECOLOR (1UL << 31)
    uint8_t  font;    /**< Default font to use */
    uint8_t  color;   /**< Default color to use */
    uint8_t  verbose; /**< Program verbosity level */
} opts_t;

static opts_t opts = {
    .fd_in  = STDIN_FILENO,
    .fd_out = STDOUT_FILENO,
    .flags  = 0,
    .font   = IWII_FONT_ELITE
};

static int _handle_char(char c);
static int _handle_args(int argc, char **const argv);

#define BUFF_SZ 64

int main(int argc, char **argv) {
    (void)opts;

    if(_handle_args(argc, argv)) {
        return -1;
    }

    char *buff = malloc(BUFF_SZ);
    if(buff == NULL) {
        fprintf(stderr, "Could not allocate space for input buffer: %s\n", strerror(errno));
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

    return 0;

main_fail:
    free(buff);
    return -1;
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
                switch(sgr) {
                    default:
                        /* Unsupported SGR */
                        goto ansi_error;
                }
            }
            ansi_pos = 0;
        }
    } else if(c == '\033') {
        ansi_buf[0] = c;
        ansi_pos = 1;
    } else {
        write(opts.fd_out, &c, 1);
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
    puts("ansi2iwii: Convert ANSI escape codes to Apple ImageWriter II escape codes\n\n");

    puts("Options:\n"
         "  -h, --help             Display this help message\n"
         "  -v, --verbose[=LEVEL]  Increase verbosity, can be supplied multiple times, or desired verbosity can be directly supplied\n"
         "  -i, --input=FILE       Read input from FILE, use `-` for stdin (default)\n"
         "  -o, --output=FILE      (experimental) Write output to FILE, use `-` for stdout (default)\n"
         "  -c, --color            (experimental) Enable support for color\n"
         "  -f, --font=FONT        Set default font to use:\n"
         "                           0: Extended\n"
         "                           1: Pica\n"
         "                           2: Elite\n"
         "                           3: Semicondensed\n"
         "                           4: Condensed\n"
         "                           5: Ultracondensed\n"
         "                           6: Pica proportional\n"
         "                           7: Elite proportional\n"
        );

    exit(0);
}

static const struct option prog_options[] = {
    { "help",    no_argument,       NULL, 'h' },
    { "verbose", optional_argument, NULL, 'v' },
    { "input",   required_argument, NULL, 'i' },
    { "output",  required_argument, NULL, 'o' },
    { "color",   no_argument,       NULL, 'c' },
    { "font",    required_argument, NULL, 'f' },
    { NULL, 0, NULL, 0 }
};

static int _handle_args(int argc, char **const argv) {
    int c;
    while ((c = getopt_long(argc, argv, "hv::i:o:cf:", prog_options, NULL)) >= 0) {
        switch(c) {
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
                    opts.fd_in = STDOUT_FILENO;
                } else {
                    opts.fd_in = open(optarg, O_WRONLY);
                    if(opts.fd_in < 0) {
                        fprintf(stderr, "Could not open output `%s`: %s\n", optarg, strerror(errno));
                        return -1;
                    }
                }
                break;
            case 'c':
                opts.flags |= OPT_FLAG_ENABLECOLOR;
                break;
            case 'f':
                if(!isdigit(optarg[0])) {
                    fprintf(stderr, "Font selection must be a number from 0 to 7!\n");
                    return -1;
                }
                opts.font = strtoul(optarg, NULL, 10);
                if(opts.font > 7) {
                    fprintf(stderr, "Font selection must be a number from 0 to 7!\n");
                    return -1;
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



