/* Native services of OPEN.EXE (the opening) where its assembly library
 * differs from MAIN's. Everything OPEN's C calls in its KOEI library that is
 * byte-identical to MAIN's (checked instruction by instruction, see
 * port/README.md "OPEN.EXE") uses the MAIN service unchanged (vgalib.c,
 * syslib.c, fileio.c, ems.c, input.c, fm.c).
 *
 * Far (segment, offset) arguments stay segment:offset here: the routines
 * address the data arena with 16-bit offset arithmetic, as the original
 * string instructions did (an offset that runs past FFFFh wraps to 0 in the
 * same segment).
 */
#ifndef PORT_OPEN_SERVICES_H
#define PORT_OPEN_SERVICES_H

#include <stdint.h>

/* ---- OCOMPOS (src/open/asm/ocompos.asm, _TEXT 1808..1E47) --------------- */
/* 1B56: save the w-byte x h-line screen rectangle at (x byte, y) into
 * seg:off, 4 bytes (planes 0..3) per byte column, rows consecutive. */
void opeu_open_bitmap_save(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                           uint16_t segment, uint16_t offset);
/* 1C2F: copy count/2 words from src_seg:src_off to seg:off (REP MOVSW). */
void opeu_open_far_copy(uint16_t source_segment, uint16_t source_offset,
                        uint16_t segment, uint16_t offset, uint16_t count);
/* 1C74: OR a save-format sprite (w byte columns, h rows) into a buffer. */
void opeu_open_buf_or(uint16_t segment, uint16_t offset, uint16_t w, uint16_t h,
                      uint16_t source_segment, uint16_t source_offset);
/* 1C94: AND every 8 buffer bytes with the mask bytes m0 m1 m0 m1 ... */
void opeu_open_buf_and_mask(uint16_t segment, uint16_t offset, uint16_t w, uint16_t h,
                            uint16_t mask_segment, uint16_t mask_offset);
/* 1E2F: one dissolve step: 288 rows of 304 pixels from the save-format
 * buffer at seg:off to the screen at (x byte, y), through the 16-bit pixel
 * mask `pattern`, rotated left by one bit per row. */
void opeu_open_dissolve_blit(uint16_t x, uint16_t y, uint16_t segment, uint16_t offset,
                             uint16_t pattern);

/* ---- OVGALIB OPEN-only (src/open/asm/ovgalib.asm) ------------------------ */
/* 09B7: an empty far routine (OPEN's init calls it with (1, 0)). */
void koei_09b7(uint16_t a, uint16_t b);

/* ---- OSYSLIB OPEN-only (src/open/asm/osyslib.asm, _TEXT 1310..13F9) ------ */
/* 133E: INT 66h AX=F001h, BX=ticks: start the effect-channel countdown. */
void opeu_open_sfx_play(uint16_t ticks);
/* 134B: INT 66h AX=F000h: the effect-channel countdown (xchg bx,ax). */
uint16_t opeu_open_sfx_busy(void);
/* 13E0: INT 66h AH=0 with DS:DX = the song bank; 1 = driver open, 0 = not. */
int16_t opeu_open_sound_open(uint16_t offset, uint16_t segment);

#endif
