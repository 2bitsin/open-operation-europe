/* Minimal dependency-free PNG writer (8-bit RGB, stored deflate blocks). */
#ifndef OPEU_PNG_WRITE_H
#define OPEU_PNG_WRITE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* rgb: w*h*3 bytes, row-major, no padding.  Returns 0 on success, -1 on error. */
int png_write_rgb(const char *path, const uint8_t *rgb, int w, int h);

#ifdef __cplusplus
}
#endif
#endif
