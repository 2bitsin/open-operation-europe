/* Native services of OPEN.EXE (the opening) where its assembly library
 * differs from MAIN's. Everything OPEN's C calls in its KOEI library that is
 * byte-identical to MAIN's (checked instruction by instruction, see
 * docs/port.md "OPEN.EXE") uses the MAIN service unchanged (vgalib.c,
 * syslib.c, fileio.c, ems.c, input.c, fm.c).
 *
 * A far argument is the 64 KiB its segment register addresses and an offset
 * in it: the routines use 16-bit offset arithmetic, as the original string
 * instructions did (an offset that runs past FFFFh wraps to 0 in the same
 * segment).
 */
#pragma once

#include <cstdint>
#include <span>

namespace opeu::services {
// The bytes a segment register addresses.
using FarSegment = std::span<std::uint8_t, 0x10000>;
// A far address: the segment's bytes and an offset in them.
struct FarBytes {
  FarSegment    segment;
  std::uint16_t offset;
};
} // namespace opeu::services

/* ---- OCOMPOS (src/open/asm/ocompos.asm, _TEXT 1808..1E47) --------------- */
/* 1B56: save the w-byte x h-line screen rectangle at (x byte, y) into
 * the destination, 4 bytes (planes 0..3) per byte column, rows consecutive. */
void opeu_open_bitmap_save(uint16_t x, uint16_t y, uint16_t w, uint16_t h, opeu::services::FarBytes destination);
/* 1C2F: copy count/2 words from source to destination (REP MOVSW). */
void opeu_open_far_copy(opeu::services::FarBytes source, opeu::services::FarBytes destination, uint16_t count);
/* 1C74: OR a save-format sprite (w byte columns, h rows) into a buffer. */
void opeu_open_buf_or(opeu::services::FarBytes target, uint16_t w, uint16_t h, opeu::services::FarBytes source);
/* 1C94: AND every 8 buffer bytes with the mask bytes m0 m1 m0 m1 ... */
void opeu_open_buf_and_mask(opeu::services::FarBytes target, uint16_t w, uint16_t h, opeu::services::FarBytes mask);
/* 1E2F: one dissolve step: 288 rows of 304 pixels from the save-format
 * buffer at source to the screen at (x byte, y), through the 16-bit pixel
 * mask `pattern`, rotated left by one bit per row. */
void opeu_open_dissolve_blit(uint16_t x, uint16_t y, opeu::services::FarBytes source, uint16_t pattern);

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
