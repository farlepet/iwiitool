#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "iwii.h"
#include "iwii_gfx.h"
#include "iwiitool.h"


typedef struct opts_struct {
/* I/O Config */
    int      fd_img;    /**< Image file descriptor */
    int      fd_out;    /**< Output file descriptor */
    uint8_t  flow;      /**< Flow control method to use */
#define OPT_FLOW_NONE          (0)
#define OPT_FLOW_XONXOFF       (1)
#define OPT_FLOW_RTSCTS        (2)
    speed_t  baud;      /**< Baud rate to use */

/* GFX Config */
    uint8_t h_dpi;      /**< Horizontal dots-per-inch */
    uint8_t v_dpi;      /**< Vertical dots-per-inch */
} opts_t;

static opts_t opts = {
/* I/O Config */
    .fd_img    = STDIN_FILENO,
    .fd_out    = STDOUT_FILENO,
    .flow      = OPT_FLOW_XONXOFF,
    .baud      = B9600,

/* GFX config */
    .h_dpi     = 72,
    .v_dpi     = 72
};

static int _setup_serial(void);
static int _handle_args(int argc, char **const argv);

#define BUFF_SZ 64

int iwiigfx(int argc, char **argv) {
    if(_handle_args(argc, argv)) {
        return -1;
    }

    if(_setup_serial()) {
        close(opts.fd_img);
        close(opts.fd_out);
        return -1;
    }

    if(iwii_gfx_init(opts.fd_out, opts.h_dpi, opts.v_dpi)) {
        goto main_fail;
    }
    
    if(iwii_gfx_print_bmp(opts.fd_out, opts.fd_img)) {
        goto main_fail;
    }

    close(opts.fd_img);
    close(opts.fd_out);

    return 0;

main_fail:
    close(opts.fd_img);
    close(opts.fd_out);

    return -1;
}

/* @todo Move this to the IWII library, since it is shared */
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


static void _help(void) {
    puts("iwiigfx: Print B&W and color images using an ImageWriter II\n");

    puts("Basic Options:\n"
         "  -i, --image=FILE          Read image from FILE, use `-` for stdin (default)\n"
         "                            Image must be in BMP format, with no more than 8 used colors,\n"
         "                            which must match those in the provided palette.bmp.\n"
         "  -o, --output=FILE         Write output to FILE, use `-` for stdout (default)\n"
         "  -b, --baud=RATE           Set baud rate to use when output is set to the printer's serial\n"
         "                            port. Values 300, 1200, 2400, and 9600 (default) are accepted\n"
         "  -F, --flow=MODE           Set flow control mode when using serial as output\n"
         "                              0: None\n"
         "                              1: XON/XOFF (default)\n"
         "                              2: RTS/CTS\n"
         "\n"
         "Graphics Options:\n"
         "  <TODO>\n"
         "\n"
         "Miscellaneous:\n"
         "  -h, --help                Display this help message\n"
        );

    exit(0);
}

static const struct option prog_options[] = {
    /* Basic Options */
    { "image",            required_argument, NULL, 'i' },
    { "output",           required_argument, NULL, 'o' },
    { "baud",             required_argument, NULL, 'b' },
    { "flow",             required_argument, NULL, 'F' },
    /* Graphics Options */
    /* Miscellaneous */
    { "help",             no_argument,       NULL, 'h' },
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
    while ((c = getopt_long(argc, argv, "i:o:b:F:"
                                        "h", prog_options, NULL)) >= 0) {
        switch(c) {
            case 'i':
                if(!strcmp(optarg, "-")) {
                    opts.fd_img = STDIN_FILENO;
                } else {
                    opts.fd_img = open(optarg, O_RDONLY);
                    if(opts.fd_img < 0) {
                        fprintf(stderr, "Could not open image `%s`: %s\n", optarg, strerror(errno));
                        return -1;
                    }
                }
                break;
            case 'o':
                if(!strcmp(optarg, "-")) {
                    opts.fd_out = STDOUT_FILENO;
                } else {
                    opts.fd_out = open(optarg, O_WRONLY | O_NOCTTY);
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

            case 'h':
                _help();
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

