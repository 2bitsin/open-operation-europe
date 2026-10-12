/* The opening's buffer compositing and screen routines that the game's library lacks: a screen save that writes
 * through a segment:offset destination (wrapping inside its segment), far copy, OR compose, AND mask, and the
 * dissolve.
 *
 * The dissolve replaces, where its mask bit is clear, every colour bit with colour bit 3 of the screen pixel, so those
 * pixels turn 0 or 15 until a later step reveals them.
 */
#include "canvas.hpp"
#include "open-services.hpp"
#include "port.hpp"

using namespace opeu::services;

/* The colour bit the dissolve keeps from the screen. */
constexpr int DISSOLVE_BIT{ 3 };

/* Byte at segment:offset of the data arena (16-bit offset arithmetic is the
 * caller's: pass the wrapped offset). */
static uint8_t* far_byte(uint16_t segment, uint16_t offset) {
  return port_far(segment, offset);
}

/* Column x (8 pixels) of game line y as a 16-bit column number, 80 per line. */
static uint16_t line_address(uint16_t x, uint16_t y) {
  return (uint16_t)((uint16_t)(y + opeu::services::detail::canvas::GAME_TOP) * 80u + x);
}

static Offset column_at(uint16_t column) {
  return Offset{ column } * 8;
}

void opeu_open_bitmap_save(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t segment, uint16_t offset) {
  uint16_t si = line_address(x, y), di = offset, rows = h;
  do {
    uint16_t columns = w;
    do {
      int p;
      for (p = 0; p < 4; p++) {
        *far_byte(segment, di) = Screen().PlaneBits(column_at(si), p);
        di                     = (uint16_t)(di + 1);
      }
      si = (uint16_t)(si + 1);
    } while (--columns);
    si = (uint16_t)(si - w + 0x50);
  } while (--rows);
}

void opeu_open_far_copy(uint16_t source_segment, uint16_t source_offset, uint16_t segment, uint16_t offset, uint16_t count) {
  uint16_t words = (uint16_t)(count >> 1), si = source_offset, di = offset;
  while (words--) { /* REP MOVSW: CX = 0 copies nothing */
    *far_byte(segment, di)                 = *far_byte(source_segment, si);
    *far_byte(segment, (uint16_t)(di + 1)) = *far_byte(source_segment, (uint16_t)(si + 1));
    si                                     = (uint16_t)(si + 2);
    di                                     = (uint16_t)(di + 2);
  }
}

void opeu_open_buf_or(uint16_t segment, uint16_t offset, uint16_t w, uint16_t h, uint16_t source_segment, uint16_t source_offset) {
  uint16_t si = source_offset, di = offset, rows = h;
  do { /* 1C54: CX = rows, inner CX = 2 * columns (LOOP) */
    uint16_t words = (uint16_t)(w << 1);
    do {
      *far_byte(segment, di)                 |= *far_byte(source_segment, si);
      *far_byte(segment, (uint16_t)(di + 1)) |= *far_byte(source_segment, (uint16_t)(si + 1));
      di                                      = (uint16_t)(di + 2);
      si                                      = (uint16_t)(si + 2);
    } while (--words);
  } while (--rows);
}

void opeu_open_buf_and_mask(uint16_t segment, uint16_t offset, uint16_t w, uint16_t h, uint16_t mask_segment, uint16_t mask_offset) {
  uint16_t si = mask_offset, di = offset, rows = h;
  do {
    uint16_t pairs = (uint16_t)(w >> 1);
    do {
      uint8_t m0 = *far_byte(mask_segment, si);
      uint8_t m1 = *far_byte(mask_segment, (uint16_t)(si + 1));
      int     k;
      for (k = 0; k < 8; k++) {
        *far_byte(segment, di) &= (k & 1) ? m1 : m0;
        di                      = (uint16_t)(di + 1);
      }
      si = (uint16_t)(si + 2);
    } while (--pairs);
  } while (--rows);
}

void opeu_open_dissolve_blit(uint16_t x, uint16_t y, uint16_t segment, uint16_t offset, uint16_t pattern) {
  uint16_t di = line_address(x, y), si = offset, dx = pattern;
  int      row;
  for (row = 0; row < 0x120; row++) {
    uint16_t row_si = si, plane_si = si, d = di;
    int      p;
    for (p = 0; p < 4; p++) {
      uint16_t s = plane_si;
      int      c;
      d = di;
      for (c = 0; c < 0x13; c++) {
        uint16_t ax = (uint16_t)(*far_byte(segment, s) << 8 | *far_byte(segment, (uint16_t)(s + 4)));
        uint16_t bx = (uint16_t)(Screen().PlaneBits(column_at(d), DISSOLVE_BIT) << 8 | Screen().PlaneBits(column_at((uint16_t)(d + 1)), DISSOLVE_BIT));
        ax          = (uint16_t)((ax & dx) | (bx & (uint16_t)~dx));
        Screen().SetPlaneBits(column_at(d), p, (uint8_t)(ax >> 8));
        Screen().SetPlaneBits(column_at((uint16_t)(d + 1)), p, (uint8_t)ax);
        d = (uint16_t)(d + 2);
        s = (uint16_t)(s + 8);
      }
      plane_si = (uint16_t)(plane_si + 1);
    }
    dx = (uint16_t)(dx << 1 | dx >> 15); /* the mask rotates left by one per row */
    di = (uint16_t)(d + 0x2a);           /* the next line: 38 columns drawn + 42 */
    si = (uint16_t)(row_si + 0x98);
  }
}

void koei_09b7(uint16_t a, uint16_t b) {
  (void)a;
  (void)b;
}
