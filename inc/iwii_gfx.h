#ifndef IWII_GFX_H
#define IWII_GFX_H

#include <stdint.h>

/**
 * @brief IWII graphics parameters/config
 */
typedef struct {
#define IWII_GFX_FLAG_RETURNTOTOP (1UL << 0) /**< Return to top of image after printing */
    uint16_t flags; /**< Flags */
    uint8_t  h_dpi; /**< Horizontal dots per inch */
    uint8_t  v_dpi; /**< Vertical dots per inch */
    unsigned h_pos; /**< Horizontal position/offset from left margin */
} iwii_gfx_params_t;

/**
 * @brief Initialize graphics portion of IWII driver
 *
 * @param fd File descriptor to write to
 * @param params 
 * @param h_dpi Desired horizontal DPI (72, 80, 96, 107, 120, 136, 144, or 160)
 * @param v_dpi Desired vertical DPI (72 or 144)
 *
 * @return 0 on success, else < 0
 */
int iwii_gfx_init(int fd, const iwii_gfx_params_t *params);

/**
 * @brief Print an image
 *
 * Image data must ordered column then row, and each byte will contain a single
 * pixel who's value matches an available ImageWriter II color. @see iwii_color_e
 *
 * @param fd File descriptor to write to
 * @param data Pointer to image data
 * @param width Width of image in pixels
 * @param height Height of image in pixels
 *
 * @return 0 on success, else < 0
 */
int iwii_gfx_print_image(int fd, const uint8_t *data, unsigned width, unsigned height);

/**
 * @brief Print an image from a BMP file
 *
 * Image can only use the 8 allowed colors, and must not use compression
 *
 * @param fd File descriptor to write to
 * @param bmp_fd File descriptor of BMP image
 *
 * @return 0 on success, else < 0
 */
int iwii_gfx_print_bmp(int fd, int bmp_fd);

/**
 * @brief Prints small test image (8x8 color pattern)
 *
 * @param fd File descriptor to write to
 *
 * @return 0 on success, else < 0
 */
int iwii_gfx_test(int fd);

#endif

