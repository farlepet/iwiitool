#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "iwii.h"
#include "iwii_gfx.h"
#include "iwiitool.h"


typedef struct opts_struct {
/* I/O Config */
    int      fd_img;    /**< Image file descriptor */
    int      fd_out;    /**< Output file descriptor */
    unsigned baud;      /**< Baud rate to use */
    uint8_t  flow;      /**< Flow control method to use */

    iwii_gfx_params_t gfx_cfg; /**< iwii_gfx configuration */
} opts_t;

static opts_t opts = {
/* I/O Config */
    .fd_img    = STDIN_FILENO,
    .fd_out    = STDOUT_FILENO,
    .flow      = IWII_FLOW_XONXOFF,
    .baud      = 9600,

/* GFX config */
    .gfx_cfg = {
        .flags     = 0,
        .h_dpi     = 72,
        .v_dpi     = 72,
        .h_pos     = 0
    }
};

static int _handle_args(int argc, char **const argv);

#define BUFF_SZ 64

int iwiigfx(int argc, char **argv) {
    if(_handle_args(argc, argv)) {
        return -1;
    }

    if(iwii_serial_init(opts.fd_out, opts.flow, opts.baud)) {
        close(opts.fd_img);
        close(opts.fd_out);
        return -1;
    }

    if(iwii_gfx_init(opts.fd_out, &opts.gfx_cfg)) {
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
         "  -H, --hdpi=DPI            Horizontal DPI, values of 72 (default), 80, 96, 107, 120,\n"
         "                            136, 144, and 160 are supported\n"
         "  -V, --vdpi=DPI            Vertical DPI, values of 72 (default), and 144 (todo) are supported.\n"
         "  -O, --hoff=OFFSET         Set horizontal offset in dots\n"
         "  -R, --return-to-top       Return to top of image after completion\n"
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
    { "hdpi",             required_argument, NULL, 'H' },
    { "vdpi",             required_argument, NULL, 'V' },
    { "hoff",             required_argument, NULL, 'O' },
    { "return-to-top",    no_argument,       NULL, 'R' },
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
                                        "H:V:O:R"
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
                switch(baud) {
                    case 300:
                    case 1200:
                    case 2400:
                    case 9600:
                        opts.baud = baud;
                        break;
                    default:
                        fprintf(stderr, "Baud rate selection must 300, 1200, 2400, or 9600!\n");
                        return -1;
                }
            } break;
            case 'F':
                _get_number(0, 2, "Flow control selection", opts.flow);
                break;
            
            case 'H': {
                if(!isdigit(optarg[0])) {
                    fprintf(stderr, "Horizontal DPI selection must 72, 80, 96, 107, 120, 136, 144, or 160!\n");
                    return -1;
                }
                unsigned dpi = strtoul(optarg, NULL, 10);
                switch(dpi) {
                    case 72:
                    case 80:
                    case 96:
                    case 107:
                    case 120:
                    case 136:
                    case 144:
                    case 160:
                        opts.gfx_cfg.h_dpi = dpi;
                        break;
                    default:
                        fprintf(stderr, "Horizontal DPI selection must 72, 80, 96, 107, 120, 136, 144, or 160!\n");
                        return -1;
                }
            } break;
            case 'V': {
                if(!isdigit(optarg[0])) {
                    fprintf(stderr, "Vertical DPI selection must 72 or 144!\n");
                    return -1;
                }
                unsigned dpi = strtoul(optarg, NULL, 10);
                switch(dpi) {
                    case 72:
                    case 144:
                        opts.gfx_cfg.v_dpi = dpi;
                        break;
                    default:
                        fprintf(stderr, "Vertical DPI selection must 72 or 144!\n");
                        return -1;
                }
            } break;
            case 'O':
                _get_number(0, 9999, "Horizontal offset", opts.gfx_cfg.h_pos);
                break;
            case 'R':
                opts.gfx_cfg.flags |= IWII_GFX_FLAG_RETURNTOTOP;
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

