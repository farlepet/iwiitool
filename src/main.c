#include <libgen.h>
#include <stdio.h>
#include <string.h>

#include "iwiitool.h"

int main(int argc, char **argv) {
    const char *prog = basename(argv[0]);
    if(!strcmp(prog, "ansi2iwii")) {
        return ansi2iwii(argc, argv);
    } else if(!strcmp(prog, "iwiigfx")) {
        return iwiigfx(argc, argv);
    }

    if(argc < 2) {
        fprintf(stderr, "Tool name required! Available tools:\n"
                        "  ansi2iwii: Reformat ANSI-formatted text to send to an ImageWriter II\n"
                        "  iwiigfx:   Print B&W and color images on an ImageWriter II\n");
        return -1;
    }

    if(!strcmp(argv[1], "ansi2iwii")) {
        return ansi2iwii(argc - 1, &argv[1]);
    } else if(!strcmp(argv[1], "iwiigfx")) {
        return iwiigfx(argc - 1, &argv[1]);
    } else {
        fprintf(stderr, "Unrecognized tool name `%s`!\n", argv[1]);
        return -1;
    }

    return 0;
}

