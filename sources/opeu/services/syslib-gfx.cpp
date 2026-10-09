/* Native C reimplementation of the drawing routines of KOEI's SYSLIB
 * (MAIN.EXE src/main/asm/syslib.asm, image 34B8..3FCD).  See syslib_gfx.h.
 *
 * Every routine below states the original address and reproduces on the
 * four VRAM planes (vga_plane, vgalib.h) the effect of the original's VGA
 * programming.  All VRAM writes in this module are done in write mode 0
 * with the data rotate/function cleared and bit mask FF (each routine calls
 * vga_reset_gc first), so a write stores the CPU byte unchanged into every
 * plane enabled in the map mask; reads return the plane chosen by the read
 * map.  None of these routines depends on graphics-controller state left by
 * an earlier routine.  They do leave the map mask at 08h and (reading
 * routines) the read map at 3, which vga_reset_gc does not reset.
 *
 * Address arithmetic is 16-bit as in the original (VRAM offsets wrap at
 * 64 KiB); loop counters follow the x86 instructions used (`loop` and
 * `dec/jnz` with a zero count run 65536 times; `rep` with zero runs none).
 */
#include "syslib-gfx.hpp"
#include "vgalib.hpp"

static uint16_t screen_offset(uint16_t x, uint16_t y) {
  return (uint16_t)((uint16_t)(y + 40) * 80u + x);
}

static uint32_t loop_count(uint16_t n) {
  return n ? n : 0x10000u;
}

/* ---- battle-cell save/restore (26 lines x 5 bytes) ---------------------- */

/* 34B8: for each of the 26 lines, planes 0..3 in turn (map mask 1,2,4,8):
 * movsw/movsw/movsb of 5 source bytes, di -= 5; then di += 4Bh. */
void opeu_vram_restore(uint16_t x, uint16_t y, const uint8_t* vram) {
  uint16_t di = screen_offset(x, y);
  int row, p, i;
  vga_reset_gc();
  for (row = 0; row < 26; row++) {
    for (p = 0; p < 4; p++)
      for (i = 0; i < 5; i++) vga_plane[p][(uint16_t)(di + i)] = *vram++;
    di = (uint16_t)(di + 80);
  }
  vga_dirty = 1;
}

/* 354F: per line and plane p (read map p, map mask 1<<p):
 *   [di]   = (VRAM[di] & F0) | (s[0] >> 4)
 *   [di+k] = (s[k-1] << 4) | (s[k] >> 4)          k = 1..4  (rol ax,4)
 *   [di+5] = (VRAM[di+5] & 0F) | (s[4] << 4)
 * The read before each partial byte uses the plane being written, so the
 * outer half-bytes of every plane are preserved. */
void opeu_vram_restore_odd(uint16_t x, uint16_t y, const uint8_t* vram) {
  uint16_t di = screen_offset(x, y);
  int row, p, k;
  for (row = 0; row < 26; row++) {
    for (p = 0; p < 4; p++) {
      uint8_t* pl = vga_plane[p];
      pl[di] = (uint8_t)((pl[di] & 0xF0) | (vram[0] >> 4));
      for (k = 1; k <= 4; k++) pl[(uint16_t)(di + k)] = (uint8_t)((vram[k - 1] << 4) | (vram[k] >> 4));
      pl[(uint16_t)(di + 5)] = (uint8_t)((pl[(uint16_t)(di + 5)] & 0x0F) | (vram[4] << 4));
      vram += 5;
    }
    di = (uint16_t)(di + 80);
  }
  vga_dirty = 1;
}

/* 37E4: per line, planes 0..3 (read map 0..3): 5 bytes each. */
void opeu_vram_save(uint16_t x, uint16_t y, uint8_t* vram) {
  uint16_t si = screen_offset(x, y);
  int row, p, i;
  for (row = 0; row < 26; row++) {
    for (p = 0; p < 4; p++)
      for (i = 0; i < 5; i++) *vram++ = vga_plane[p][(uint16_t)(si + i)];
    si = (uint16_t)(si + 80);
  }
}

/* 3882: per line and plane, 5 bytes (v[k] << 4) | (v[k+1] >> 4), k = 0..4,
 * from the 6 screen bytes at si (rol ax,4 of the word at si+k). */
void opeu_vram_save_odd(uint16_t x, uint16_t y, uint8_t* vram) {
  uint16_t si = screen_offset(x, y);
  int row, p, k;
  for (row = 0; row < 26; row++) {
    for (p = 0; p < 4; p++) {
      const uint8_t* pl = vga_plane[p];
      for (k = 0; k < 5; k++) *vram++ = (uint8_t)((pl[(uint16_t)(si + k)] << 4) | (pl[(uint16_t)(si + k + 1)] >> 4));
    }
    si = (uint16_t)(si + 80);
  }
}

/* ---- cell-buffer composition (system memory only) ----------------------- */

/* 3A04: 104h words: or [es:di],ax. */
void opeu_vram_draw(uint8_t* vram, const uint8_t* src) {
  unsigned i;
  for (i = 0; i < SYSLIB_CELL_BYTES; i++) vram[i] |= src[i];
}

/* 3A2A: same loop with a near source. */
void opeu_vram_draw_near(uint8_t* vram, const uint8_t* src) {
  opeu_vram_draw(vram, src);
}

/* Shift a glyph byte right by x & 7 across a byte pair, as `ror ax,cl` with
 * AH = 0 stores it: [di] = g >> c, [di+1] = g << (8 - c). */
static void glyph_pair(uint8_t g, unsigned c, uint8_t* lo, uint8_t* hi) {
  uint16_t ax = (uint16_t)(((unsigned)g >> c) | ((unsigned)g << (16 - c)));
  *lo = (uint8_t)ax;
  *hi = (uint8_t)(ax >> 8);
}

/* Word OR / AND at a byte address of the cell buffer.  The original uses
 * word operations, so with a glyph in byte column 4 the high byte lands in
 * byte 0 of the next plane block (or the next row): reproduced byte-wise. */
static void or16(uint8_t* p, uint8_t lo, uint8_t hi) {
  p[0] |= lo;
  p[1] |= hi;
}
static void and16(uint8_t* p, uint8_t lo, uint8_t hi) {
  p[0] &= lo;
  p[1] &= hi;
}

/* 3A4D: di = vram + y*20 + x/8, 6 glyph rows, di += 14h per row.
 * color low byte 0: plane0 &= ~g, plane1 |= g, plane2/3 &= ~g (colour 2);
 * otherwise plane0 |= g, planes 1-3 &= ~g (colour 1). */
void opeu_vram_glyph_color(uint8_t* vram, const uint8_t* glyph, uint16_t x, uint16_t y, uint16_t color) {
  uint8_t* di = vram + (uint16_t)((uint16_t)(y * 20u) + (x >> 3));
  unsigned c = x & 7;
  int row;
  for (row = 0; row < 6; row++, di += 20) {
    uint8_t lo, hi;
    glyph_pair(*glyph++, c, &lo, &hi);
    if ((uint8_t)color == 0) {
      and16(di, (uint8_t)~lo, (uint8_t)~hi);
      or16(di + 5, lo, hi);
    } else {
      or16(di, lo, hi);
      and16(di + 5, (uint8_t)~lo, (uint8_t)~hi);
    }
    and16(di + 10, (uint8_t)~lo, (uint8_t)~hi);
    and16(di + 15, (uint8_t)~lo, (uint8_t)~hi);
  }
}

/* 3AC3: planes 0-2 |= g, plane 3 &= ~g. */
void opeu_vram_glyph(uint8_t* vram, const uint8_t* glyph, uint16_t x, uint16_t y) {
  uint8_t* di = vram + (uint16_t)((uint16_t)(y * 20u) + (x >> 3));
  unsigned c = x & 7;
  int row;
  for (row = 0; row < 6; row++, di += 20) {
    uint8_t lo, hi;
    glyph_pair(*glyph++, c, &lo, &hi);
    or16(di, lo, hi);
    or16(di + 5, lo, hi);
    or16(di + 10, lo, hi);
    and16(di + 15, (uint8_t)~lo, (uint8_t)~hi);
  }
}

/* 3B1E: 26 rows: 5 mask bytes ANDed into the four plane blocks. */
void opeu_vram_load(uint8_t* vram, const uint8_t* mask) {
  int row, i;
  for (row = 0; row < 26; row++, vram += 20, mask += 5)
    for (i = 0; i < 5; i++) {
      vram[i] &= mask[i];
      vram[i + 5] &= mask[i];
      vram[i + 10] &= mask[i];
      vram[i + 15] &= mask[i];
    }
}

/* ---- VRAM fills and copies ---------------------------------------------- */

/* 3B77: map mask 8, write mode 0: rep stosb of the low byte of color, w
 * bytes per line, h lines (`loop`: h = 0 means 65536).  No clipping. */
void koei_3b77(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
  uint16_t di = screen_offset(x, y);
  uint32_t rows = loop_count(h), r;
  uint16_t i;
  vga_reset_gc();
  for (r = 0; r < rows; r++) {
    for (i = 0; i < w; i++) vga_plane[3][di++] = (uint8_t)color;
    di = (uint16_t)(di + 80 - w);
  }
  vga_dirty = 1;
}

/* 3BC5: for planes 0..3 (read map p, map mask 1<<p), 24 lines of
 * movsw + movsb from (x1, y1) to (x2, y2), forward.  A movsw reads both
 * bytes before writing them, which matters only for overlapping blocks. */
void koei_3bc5(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
  uint16_t src = screen_offset(x1, y1), dst = screen_offset(x2, y2);
  int p, row;
  vga_reset_gc();
  for (p = 0; p < 4; p++) {
    uint8_t* pl = vga_plane[p];
    uint16_t si = src, di = dst;
    for (row = 0; row < 24; row++) {
      uint8_t a = pl[si], b = pl[(uint16_t)(si + 1)];
      pl[di] = a;
      pl[(uint16_t)(di + 1)] = b;
      pl[(uint16_t)(di + 2)] = pl[(uint16_t)(si + 2)];
      si = (uint16_t)(si + 80);
      di = (uint16_t)(di + 80);
    }
  }
  vga_dirty = 1;
}

/* 3CC4: save w bytes x h lines from screen offset si into buf, plane-major
 * (read map 0..3); returns the bytes stored per plane. */
static uint32_t save_rect(uint16_t si, uint16_t w, uint16_t h, uint8_t* buf) {
  uint32_t rows = loop_count(h), r, k = 0;
  int p;
  uint16_t i;
  for (p = 0; p < 4; p++) {
    uint16_t s = si;
    for (r = 0; r < rows; r++) {
      for (i = 0; i < w; i++) buf[k++] = vga_plane[p][s++];
      s = (uint16_t)(s + 80 - w);
    }
  }
  return k / 4;
}

/* 3D67: restore the plane-major buffer to screen offset di (map mask 1<<p). */
static void restore_rect(uint16_t di, uint16_t w, uint16_t h, const uint8_t* buf) {
  uint32_t rows = loop_count(h), r, k = 0;
  int p;
  uint16_t i;
  vga_reset_gc();
  for (p = 0; p < 4; p++) {
    uint16_t d = di;
    for (r = 0; r < rows; r++) {
      for (i = 0; i < w; i++) vga_plane[p][d++] = buf[k++];
      d = (uint16_t)(d + 80 - w);
    }
  }
  vga_dirty = 1;
}

/* 3E0A: see syslib_gfx.h.  n = w * h (16-bit `mul`, low word); the AND and
 * OR passes use `loop` with cx = n. */
void opeu_sprite_compose(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t* and_mask,
                         const uint8_t* or_image, uint8_t* buffer, uint16_t flags) {
  uint16_t at = screen_offset(x, y);
  uint16_t n16 = (uint16_t)((uint32_t)w * h);
  uint32_t n = loop_count(n16), i;
  int p;
  save_rect(at, w, h, buffer);
  for (p = 0; p < 4; p++)
    for (i = 0; i < n; i++) buffer[p * (uint32_t)n16 + i] &= and_mask[i];
  for (i = 0; i < n; i++) {
    buffer[i] |= *or_image++;
    buffer[i + n16] |= *or_image++;
    buffer[i + 2u * n16] |= *or_image++;
    if (flags & 1) buffer[i + 3u * n16] |= *or_image++;
  }
  restore_rect(at, w, h, buffer);
}

/* 3EA2: AND one plane block of the buffer (W bytes x h lines) with the mask
 * (W-1 bytes per line) shifted right by c pixels; ones are shifted in. */
static uint8_t* and_shifted(uint8_t* d, const uint8_t* m, unsigned c, uint16_t W, uint16_t h) {
  uint32_t rows = loop_count(h), r;
  for (r = 0; r < rows; r++) {
    uint16_t dx = (uint16_t)(W - 2);       /* W = 1: 65535 (as the original) */
    *d++ &= (uint8_t)((0xFF00u | *m) >> c);
    for (; dx; dx--, m++) *d++ &= (uint8_t)((((unsigned)m[0] << 8) | m[1]) >> c);
    if (c) *d &= (uint8_t)((((unsigned)m[0] << 8) | 0xFFu) >> c);
    d++;
    m++;
  }
  return d;
}

/* 3EE0: OR one plane of the interleaved image (stride 3 + flags) into the
 * next plane block, shifted right by c pixels; zeros are shifted in. */
static uint8_t* or_shifted(uint8_t* d, const uint8_t* s, unsigned c, uint16_t W, uint16_t h, uint16_t stride) {
  uint32_t rows = loop_count(h), r;
  for (r = 0; r < rows; r++) {
    uint16_t dx = (uint16_t)(W - 2);
    *d++ |= (uint8_t)(*s >> c);
    for (; dx; dx--) {
      unsigned hi = *s;
      s += stride;
      *d++ |= (uint8_t)(((hi << 8) | *s) >> c);
    }
    if (c) *d |= (uint8_t)(((unsigned)*s << 8) >> c);
    d++;
    s += stride;
  }
  return d;
}

/* 3F26: x, w in pixels; W = (w >> 3) + 1 bytes; si = (y+40)*80 + x/8.
 * Save, AND the same mask into each of the 4 plane blocks, OR image planes
 * 0, 1, 2 (stride 3 + flags, the whole flags word), then plane 3 and the
 * restore only when flags bit 0 is set.  With bit 0 clear the original
 * jumps back to 3F7A forever (re-ORing planes 0-2, which changes nothing
 * after the first pass) and never returns or writes the screen; this port
 * performs that first pass and returns without touching VRAM. */
void koei_3f26(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t* and_mask, const uint8_t* or_image,
               uint8_t* buffer, uint16_t flags) {
  uint16_t at = (uint16_t)((uint16_t)(y + 40) * 80u + (x >> 3));
  uint16_t W = (uint16_t)((w >> 3) + 1);
  uint16_t stride = (uint16_t)(3 + flags);
  unsigned c = x & 7;
  uint8_t* d = buffer;
  int p;
  save_rect(at, W, h, buffer);
  for (p = 0; p < 4; p++) d = and_shifted(d, and_mask, c, W, h);
  d = buffer;
  for (p = 0; p < 3; p++) d = or_shifted(d, or_image + p, c, W, h, stride);
  if (!(flags & 1)) return;                 /* original: endless loop */
  or_shifted(d, or_image + 3, c, W, h, stride);
  restore_rect(at, W, h, buffer);
}
