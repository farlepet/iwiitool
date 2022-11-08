#ifndef IWII_GFX_H
#define IWII_GFX_H

#include <stdint.h>

/**
 * @brief Initialize graphics portion of IWII driver
 *
 * @param fd File descriptor to write to
 * @param h_dpi Desired horizontal DPI (72, 80, 96, 107, 120, 136, 144, or 160)
 * @param v_dpi Desired vertical DPI (72 or 144)
 *
 * @return 0 on success, else < 0
 */
int iwii_gfx_init(int fd, unsigned h_dpi, unsigned v_dpi);

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

