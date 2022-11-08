#ifndef BMP_H
#define BMP_H

#include <stdint.h>

#pragma pack(push)
#pragma pack(1)

typedef struct {
#define BMP_SIGNATURE (0x4D42) /**< BMP file signature (assuming little-endian) */
    uint16_t signature;
    uint32_t file_sz;
    uint16_t _reserved[2];
    uint32_t img_offset;
} bmp_file_header_t;

typedef struct {
    uint32_t dib_size;
    uint32_t width;
    uint32_t height;
    uint16_t n_planes;
    uint16_t bpp;
#define BMP_COMPRESSION_RGB            ( 0)
#define BMP_COMPRESSION_RLE8           ( 1)
#define BMP_COMPRESSION_RLE4           ( 2)
#define BMP_COMPRESSION_BITFIELDS      ( 3)
#define BMP_COMPRESSION_JPEG           ( 4)
#define BMP_COMPRESSION_PNG            ( 5)
#define BMP_COMPRESSION_ALPHABITFIELDS ( 6)
#define BMP_COMPRESSION_CMYZ           (11)
#define BMP_COMPRESSION_CMYKRLE8       (12)
#define BMP_COMPRESSION_CMYKRLE4       (13)
    uint32_t compression;
    uint32_t image_sz;
    uint32_t h_pix_per_m;
    uint32_t v_pix_per_m;
    uint32_t n_colors;
    uint32_t important_colors;
} bmp_dib_header_t;

typedef struct {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
    uint8_t _reserved;
} bmp_color_entry_t;

#pragma pack(pop)

typedef struct {
    bmp_file_header_t  file_head; /**< File header */
    bmp_dib_header_t   dib_head;  /**< DIB header */
    bmp_color_entry_t *palette;   /**< Color palette */
    uint8_t           *data;      /**< Pixel data */

    size_t             row_sz;    /**< Size of single row of data, in bytes */
    size_t             data_sz;   /**< Size of data region, in bytes, including padding */
} bmp_hand_t;

int bmp_load_file(bmp_hand_t *hand, int fd);

void bmp_destroy(bmp_hand_t *hand);

/**
 * @brief Get a single pixel from a bitmap image
 *
 * @param hand BMP handle
 * @param x X position (left = 0)
 * @param y Y position (top = 0)
 * @return < 0 on failure, palette index on success
 */
int bmp_get_pixel(bmp_hand_t *hand, uint32_t x, uint32_t y);

#endif

