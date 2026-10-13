// The opening's buffer compositing and screen routines that the game's library lacks: a screen save that writes
// through a segment:offset destination (wrapping inside its segment), far copy, OR compose, AND mask, and the
// dissolve.
//
// The dissolve replaces, where its mask bit is clear, every colour bit with colour bit 3 of the screen pixel, so those
// pixels turn 0 or 15 until a later step reveals them.
#include "canvas.hpp"
#include "open-services.hpp"

using namespace opeu::services;

// The colour bit the dissolve keeps from the screen.
constexpr int DISSOLVE_BIT{ 3 };

// Column x (8 pixels) of game line y as a 16-bit column number, 80 per line.
static uint16_t ScreenColumn(uint16_t x, uint16_t y) {
  return (uint16_t)((uint16_t)(y + opeu::services::detail::canvas::GAME_TOP) * 80u + x);
}

static Offset ColumnOffset(uint16_t column) {
  return Offset{ column } * 8;
}

void SaveScreenRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, FarBytes destination) {
  FarSegment const segment{ destination.segment };
  uint16_t         si = ScreenColumn(x, y), di = destination.offset, rows = h;
  do {
    uint16_t columns = w;
    do {
      int p;
      for (p = 0; p < 4; p++) {
        segment[di] = Screen().PlaneBits(ColumnOffset(si), p);
        di          = (uint16_t)(di + 1);
      }
      si = (uint16_t)(si + 1);
    } while (--columns);
    si = (uint16_t)(si - w + 0x50);
  } while (--rows);
}

void CopyFarWords(FarBytes source, FarBytes destination, uint16_t count) {
  FarSegment const source_segment{ source.segment };
  FarSegment const segment{ destination.segment };
  uint16_t         words = (uint16_t)(count >> 1), si = source.offset, di = destination.offset;
  while (words--) { // REP MOVSW: CX = 0 copies nothing
    segment[di]                 = source_segment[si];
    segment[(uint16_t)(di + 1)] = source_segment[(uint16_t)(si + 1)];
    si                          = (uint16_t)(si + 2);
    di                          = (uint16_t)(di + 2);
  }
}

void OrIntoBuffer(FarBytes target, uint16_t w, uint16_t h, FarBytes source) {
  FarSegment const segment{ target.segment };
  FarSegment const source_segment{ source.segment };
  uint16_t         si = source.offset, di = target.offset, rows = h;
  do { // 1C54: CX = rows, inner CX = 2 * columns (LOOP)
    uint16_t words = (uint16_t)(w << 1);
    do {
      segment[di]                 |= source_segment[si];
      segment[(uint16_t)(di + 1)] |= source_segment[(uint16_t)(si + 1)];
      di                           = (uint16_t)(di + 2);
      si                           = (uint16_t)(si + 2);
    } while (--words);
  } while (--rows);
}

void AndWithMask(FarBytes target, uint16_t w, uint16_t h, FarBytes mask) {
  FarSegment const segment{ target.segment };
  FarSegment const mask_segment{ mask.segment };
  uint16_t         si = mask.offset, di = target.offset, rows = h;
  do {
    uint16_t pairs = (uint16_t)(w >> 1);
    do {
      uint8_t m0 = mask_segment[si];
      uint8_t m1 = mask_segment[(uint16_t)(si + 1)];
      int     k;
      for (k = 0; k < 8; k++) {
        segment[di] &= (k & 1) ? m1 : m0;
        di           = (uint16_t)(di + 1);
      }
      si = (uint16_t)(si + 2);
    } while (--pairs);
  } while (--rows);
}

void DissolveStep(uint16_t x, uint16_t y, FarBytes source, uint16_t pattern) {
  FarSegment const segment{ source.segment };
  uint16_t         di = ScreenColumn(x, y), si = source.offset, dx = pattern;
  int              row;
  for (row = 0; row < 0x120; row++) {
    uint16_t row_si = si, plane_si = si, d = di;
    int      p;
    for (p = 0; p < 4; p++) {
      uint16_t s = plane_si;
      int      c;
      d = di;
      for (c = 0; c < 0x13; c++) {
        uint16_t ax = (uint16_t)(segment[s] << 8 | segment[(uint16_t)(s + 4)]);
        uint16_t bx = (uint16_t)(Screen().PlaneBits(ColumnOffset(d), DISSOLVE_BIT) << 8 | Screen().PlaneBits(ColumnOffset((uint16_t)(d + 1)), DISSOLVE_BIT));
        ax          = (uint16_t)((ax & dx) | (bx & (uint16_t)~dx));
        Screen().SetPlaneBits(ColumnOffset(d), p, (uint8_t)(ax >> 8));
        Screen().SetPlaneBits(ColumnOffset((uint16_t)(d + 1)), p, (uint8_t)ax);
        d = (uint16_t)(d + 2);
        s = (uint16_t)(s + 8);
      }
      plane_si = (uint16_t)(plane_si + 1);
    }
    dx = (uint16_t)(dx << 1 | dx >> 15); // the mask rotates left by one per row
    di = (uint16_t)(d + 0x2a);           // the next line: 38 columns drawn + 42
    si = (uint16_t)(row_si + 0x98);
  }
}
