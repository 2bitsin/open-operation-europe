/* OPEN file-backed DGROUP DS:0356..0417 (194 bytes): data of the helper
 * segment 062d (src/open/core/outil.c).
 *
 * The image-stream byte count, the current palette and the black palette the
 * fade helpers step between (16 RGB triples of 6-bit DAC levels 0..15 as
 * the opening uses them), a further palette and a 16x16 one-bit picture that
 * no recovered routine addresses, and the fatal-error format of the
 * open-or-die helper. All objects start at even offsets.
 */
#include "types.hpp"

uint16_t opeu_stream_count = 0;                  /* DS:0356 */
uint8_t opeu_open_palette[48] = { 0 };             /* DS:0358 current palette */
uint8_t opeu_open_black[48] = { 0 };               /* DS:0388 all black */

/* DS:03b8: unreferenced palette. */
uint8_t opeu_open_spare_palette[48] = { 0, 0,  0,  6,  4, 8,  5,  13, 3,  9, 13, 11, 11, 6,  3,  10,
                                        7, 12, 11, 15, 6, 15, 15, 15, 7,  7, 7,  1,  8,  3,  4,  10,
                                        6, 7,  10, 4,  8, 15, 6,  10, 14, 9, 12, 15, 12, 14, 15, 12 };

/* DS:03e8: unreferenced 16x16 one-bit picture, two bytes per row (most
 * significant bit leftmost): a rounded frame around an arrow. */
uint8_t opeu_open_icon[16][2] = { { 0x3f, 0xfc }, { 0x60, 0x06 }, { 0xc0, 0x03 }, { 0x80, 0x79 },
                                  { 0x80, 0x79 }, { 0x82, 0x79 }, { 0x86, 0x79 }, { 0x8f, 0xf9 },
                                  { 0x9f, 0xf9 }, { 0xbf, 0xf9 }, { 0x9f, 0xf1 }, { 0x8f, 0xe1 },
                                  { 0x86, 0x01 }, { 0xc2, 0x03 }, { 0x60, 0x06 }, { 0x3f, 0xfc } };

char opeu_open_error_format[] = "OPEN ERROR=  %s";  /* DS:0408 */
