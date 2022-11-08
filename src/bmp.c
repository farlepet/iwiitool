#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "bmp.h"


int bmp_load_file(bmp_hand_t *hand, int fd) {
    if(pread(fd, &hand->file_head, sizeof(bmp_file_header_t), 0) != sizeof(bmp_file_header_t)) {
        fprintf(stderr, "BMP: Could not read file header\n");
        return -1;
    }
    if(hand->file_head.signature != BMP_SIGNATURE) {
        fprintf(stderr, "BMP: Invalid signature: %hu\n", hand->file_head.signature);
        return -1;
    }

    {
        uint32_t dib_sz = 0;
        if((pread(fd, &dib_sz, 4, sizeof(bmp_file_header_t)) != 4) ||
           (dib_sz == 0)) {
            fprintf(stderr, "BMP: Could not read DIB size\n");
            return -1;
        }
        if(dib_sz > sizeof(bmp_dib_header_t)) {
            dib_sz = sizeof(bmp_dib_header_t);
        }
        if(pread(fd, &hand->dib_head, dib_sz, sizeof(bmp_file_header_t)) != (ssize_t)dib_sz) {
            fprintf(stderr, "BMP: Could not read DIB header\n");
            return -1;
        }
    }

    if(hand->dib_head.compression != BMP_COMPRESSION_RGB) {
        fprintf(stderr, "BMP: Unsupported compression value: %u\n", hand->dib_head.compression);
        return -1;
    }
    if(hand->dib_head.bpp > 4) {
        fprintf(stderr, "BMP: Unsupported bits-per-pixel value: %hu\n", hand->dib_head.bpp);
        return -1;
    }
    if(hand->dib_head.n_colors == 0) {
        hand->dib_head.n_colors = 1 << hand->dib_head.bpp;
    }

    {
        size_t palette_sz = hand->dib_head.n_colors * sizeof(bmp_color_entry_t);
        hand->palette = malloc(palette_sz);
        if(hand->palette == NULL) {
            fprintf(stderr, "BMP: Could not allocate memory for palette\n");
            return -1;
        }
        if(pread(fd, hand->palette, palette_sz, sizeof(bmp_file_header_t) + hand->dib_head.dib_size) != (ssize_t)palette_sz) {
            fprintf(stderr, "BMP: Could not read palette\n");
            free(hand->palette);
            return -1;
        }
    }

    hand->row_sz  = ((hand->dib_head.bpp * hand->dib_head.width + 31) / 32) * 4;
    hand->data_sz = hand->row_sz * hand->dib_head.height;
    hand->data = malloc(hand->data_sz);
    if(hand->data == NULL) {
        fprintf(stderr, "BMP: Could not allocate memory for pixel data\n");
        free(hand->palette);
        return -1;
    }
    if(pread(fd, hand->data, hand->data_sz, hand->file_head.img_offset) != (ssize_t)hand->data_sz) {
        fprintf(stderr, "BMP: Could not read pixel data\n");
        free(hand->palette);
        free(hand->data);
        return -1;
    }

    return 0;
}

void bmp_destroy(bmp_hand_t *hand) {
    free(hand->palette);
    free(hand->data);
}

int bmp_get_pixel(bmp_hand_t *hand, uint32_t x, uint32_t y) {
    /* Assuming height is positive */
    if((x >= hand->dib_head.width) || (y >= hand->dib_head.height)) {
        return -1;
    }

    unsigned byte = (x * hand->dib_head.bpp) / 8;
    uint8_t  msb  = 7 - ((x * hand->dib_head.bpp) % 8);
    uint8_t  lsb  = msb - (hand->dib_head.bpp - 1);

    uint8_t *row = &hand->data[hand->row_sz * ((hand->dib_head.height - 1) - y)];
    uint8_t  px  = (row[byte] >> lsb) & ((1 << hand->dib_head.bpp) - 1);
    
    return px;
}

