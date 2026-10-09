/* Native C reimplementation of KOEI's planar VGA library, MAIN.EXE
 * src/main/asm/vgalib.asm (_TEXT 23AE..34B7, DGROUP 3E9A..3EB9).
 *
 * See vgalib.h for the display model.  Every routine is written as the
 * effect its VGA register programming has on the four planes of real
 * hardware; comments give the original addresses and spell out the
 * hardware behaviour each step relies on.  Address arithmetic is 16-bit
 * (uint16_t), as in the original registers, so out-of-range coordinates wrap
 * inside the 64 KiB planes exactly as they did on the card.
 *
 * Hardware facts used throughout (IBM VGA, mode 12h):
 *  - a CPU read of A000:a loads the four latches with byte a of each plane;
 *  - write mode 0: each plane enabled in the sequencer map mask receives
 *    (data OP latch) under the bit mask, latch elsewhere; OP is the GC data
 *    rotate function (replace/AND/OR/XOR);
 *  - write mode 1: enabled planes receive the latches;
 *  - write mode 2: bit p of the data is replicated to all 8 bits of plane p
 *    before OP and the bit mask;
 *  - enable set/reset replaces the data of enabled planes by the set/reset
 *    colour bits.
 * vga_reset_gc (23AE), called by almost every routine, sets enable
 * set/reset 0, function replace, rotate 0, write mode 0 and bit mask FF.
 */
#include "vgalib.hpp"
#include <string.h>

uint8_t vga_plane[4][VGA_PLANE_SIZE] = { { 0 } };
uint8_t vga_dac[256 * 3];
uint8_t vga_attr[16];
uint8_t vga_pel_mask = 0xFF;
int vga_text_mode = 1;
int vga_dirty = 1;

#define P0 vga_plane[0]
#define P1 vga_plane[1]
#define P2 vga_plane[2]
#define P3 vga_plane[3]

/* (y + 40) * 80 as the original computes it (16-bit). */
static uint16_t line_addr(uint16_t y) {
  return (uint16_t)((uint16_t)(y + VGA_TOP) * VGA_PITCH);
}

/* GC data-rotate logical functions (bits 3-4 of GC index 3). */
enum { FN_REPLACE = 0x00, FN_AND = 0x08, FN_OR = 0x10, FN_XOR = 0x18 };

static uint8_t logic(uint8_t fn, uint8_t data, uint8_t latch) {
  switch (fn & 0x18) {
  case FN_AND: return (uint8_t)(data & latch);
  case FN_OR: return (uint8_t)(data | latch);
  case FN_XOR: return (uint8_t)(data ^ latch);
  default: return data;
  }
}

/* ---- 23AE vga_reset_gc ---------------------------------------------------- */

/* No register state exists in the native model; each routine below applies
 * the reset state where the original called this.  Kept so that ports of
 * other KOEI modules (SYSLIB) can call it unchanged. */
void vga_reset_gc(void) { }

/* ---- palette ------------------------------------------------------------- */

/* 23C4: INT 10h AX=1000h for BL = BH = 0..15: attribute palette identity, so
 * pixel value n selects DAC register n. */
void koei_vga_identity_palette(void) {
  int i;
  for (i = 0; i < 16; i++) vga_attr[i] = (uint8_t)i;
  vga_dirty = 1;
}

/* 23D5: load `count` DAC registers from `first` (first & 15).  Each source
 * entry is three 4-bit values (b0, b1, b2) that go to the DAC as R = b1 << 2,
 * G = b2 << 2, B = b0 << 2 (byte shifts; the DAC keeps the low 6 bits).  A
 * count above 16 becomes 16 - first; a count <= 16 is used as given, so
 * first + count may run past register 15.  The original waits for vertical
 * retrace first (3DA bit 3) and returns AX = 0.  count = 0 made the original
 * LOOP run 65536 times over a 48-byte stack buffer (a crash); here it is a
 * no-op. */
int16_t opeu_set_palette(uint8_t first, uint8_t count, const uint8_t* rgb) {
  uint8_t index = (uint8_t)(first & 0x0F);
  uint8_t n = count;
  uint8_t i;
  if (n > 0x10) n = (uint8_t)(0x10 - index);
  for (i = 0; i < n; i++, rgb += 3) {
    uint8_t* d = &vga_dac[(uint8_t)(index + i) * 3];
    d[0] = (uint8_t)((rgb[1] << 2) & 0x3F);
    d[1] = (uint8_t)((rgb[2] << 2) & 0x3F);
    d[2] = (uint8_t)((rgb[0] << 2) & 0x3F);
  }
  vga_dirty = 1;
  return 0;
}

/* ---- 2444 clip_and_address ------------------------------------------------- */

/* x, w in bytes (x <= 79, x + w <= 80), y, h in game lines (y <= 399,
 * y + h <= 400), w and h non-zero, sums without 16-bit carry.  Returns 1 and
 * the VRAM offset on success (the original also resets the GC there). */
static int clip_and_address(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t* di) {
  if (x > 0x4F || w == 0 || (uint32_t)x + w > 0x50) return 0;
  if (y > 0x18F || h == 0 || (uint32_t)y + h > 0x190) return 0;
  *di = (uint16_t)(line_addr(y) + x);
  return 1;
}

/* ---- 2498..2540 one-bit pixel writer ------------------------------------- */

/* CS-resident state of the pixel writer (2498 colour, 2499 function, 249A
 * mask-mode); it persists between calls, initially 07, 00, 00. */
static uint8_t pixel_color = 0x07;
static uint8_t pixel_function = 0x00;
static uint8_t pixel_mask_mode = 0x00;

/* 24EB: select the operation of 249B: AH 0 set, 1 XOR, 2 AND, 3 OR; AL the
 * colour.  AH > 3 changes nothing (not even the colour). */
static void pixel_select(uint8_t mode, uint8_t color) {
  switch (mode) {
  case 0:
    pixel_function = FN_OR;
    pixel_mask_mode = FN_AND;
    break;
  case 1:
    pixel_function = FN_XOR;
    pixel_mask_mode = 0;
    break;
  case 2:
    pixel_mask_mode = 0;
    pixel_function = FN_AND;
    break;
  case 3:
    pixel_function = FN_OR;
    pixel_mask_mode = 0;
    break;
  default: return;
  }
  pixel_color = color;
}

/* 249B: apply the one-bit pattern `bits` to VRAM byte `a` (GC in reset
 * state: write mode 0, bit mask FF).  With a function selected: the planes of
 * the colour (map mask = colour) receive bits OP latch.  Then, with a mask
 * mode, the other planes (map mask = ~colour) receive ~bits AND latch, the
 * latch re-read after the first write.  Net effect per pixel with a 1 bit:
 * set -> colour, XOR -> pixel ^ colour, OR -> pixel | colour; with a 0 bit:
 * AND -> pixel & ~colour.  Leaves the GC function at replace. */
static void pixel_write(uint16_t a, uint8_t bits) {
  int p;
  if (pixel_function) {
    for (p = 0; p < 4; p++)
      if (pixel_color & (1u << p)) vga_plane[p][a] = logic(pixel_function, bits, vga_plane[p][a]);
  }
  if (pixel_mask_mode) {
    uint8_t nb = (uint8_t)~bits;
    for (p = 0; p < 4; p++)
      if (!(pixel_color & (1u << p))) vga_plane[p][a] = logic(pixel_mask_mode, nb, vga_plane[p][a]);
  }
}

/* ---- 254C planar bitmap, 4 bytes per byte column -------------------------- */

/* Source: for each row, for each byte column, the bytes of planes 0, 1, 2, 3.
 * mode & 7FFF: 0 copy, 1 XOR, 2 AND, 3 OR (each plane with the latched VRAM
 * byte), 4 transparent copy (bit mask = OR of the four plane bytes, i.e. the
 * non-zero pixels; latches supply the rest).  Bit 15 (mode 0 only) draws
 * every source row on two lines and advances two lines per row; the clip
 * only checks h, so doubled rows may run past line 400.  Returns 0, or -1
 * (FFFF) when the clip fails or the mode is above 4. */
static int16_t blit4(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode, const uint8_t* s) {
  uint16_t di, m = (uint16_t)(mode & 0x7FFF), c, r;
  int p;
  if (!clip_and_address(x, y, w, h, &di) || m > 4) return -1;
  vga_dirty = 1;
  if (m == 0) {
    r = h;
    do {
      const uint8_t* row = s;
      uint16_t a = di;
      for (c = 0; c < w; c++, a++, s += 4)
        for (p = 0; p < 4; p++) vga_plane[p][a] = s[p];
      if (mode & 0x8000) {
        s = row;
        a = (uint16_t)(di + 0x50);
        for (c = 0; c < w; c++, a++, s += 4)
          for (p = 0; p < 4; p++) vga_plane[p][a] = s[p];
        di = (uint16_t)(di + 0x50);
      }
      di = (uint16_t)(di + 0x50);
    } while (--r);
    return 0;
  }
  if (m <= 3) {
    static const uint8_t fn[4] = { 0, FN_XOR, FN_AND, FN_OR };
    r = h;
    do {
      uint16_t a = di;
      for (c = 0; c < w; c++, a++, s += 4)
        for (p = 0; p < 4; p++) vga_plane[p][a] = logic(fn[m], s[p], vga_plane[p][a]);
      di = (uint16_t)(di + 0x50);
    } while (--r);
    return 0;
  }
  r = h;
  do {
    uint16_t a = di;
    for (c = 0; c < w; c++, a++, s += 4) {
      uint8_t mask = (uint8_t)(s[0] | s[1] | s[2] | s[3]);
      for (p = 0; p < 4; p++) vga_plane[p][a] = (uint8_t)((s[p] & mask) | (vga_plane[p][a] & ~mask));
    }
    di = (uint16_t)(di + 0x50);
  } while (--r);
  return 0;
}

/* 266C: blit4 with a near (DS) source. */
int16_t koei_266c(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode, const uint8_t* data) {
  return blit4(x, y, w, h, mode, data);
}

/* 2674: blit4 with a far source (segment, offset in the original). */
int16_t opeu_bitmap_restore(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode, const uint8_t* data) {
  return blit4(x, y, w, h, mode, data);
}

/* ---- 2693 planar bitmap, 3 bytes per byte column -------------------------- */

/* Source: planes 0, 1, 2 per byte column; plane 3 is not stored.
 * mode & 3FFF: 0 copy, plane 3 = 00 (or FF with bit 14; the immediates at
 * 26F8/2727 are patched); 1 XOR, 2 AND, 3 OR on planes 0-2 only (plane 3
 * keeps its contents); 4 transparent: bit mask = OR of the three bytes,
 * planes 0-2 copied and plane 3 cleared under it.  Bit 15 doubles rows as in
 * blit4 (mode 0 only).  Returns 0 or -1. */
static int16_t blit3(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode, const uint8_t* s) {
  uint16_t di, m = (uint16_t)(mode & 0x3FFF), c, r;
  int p;
  if (!clip_and_address(x, y, w, h, &di) || m > 4) return -1;
  vga_dirty = 1;
  if (m == 0) {
    uint8_t fill = (mode & 0x4000) ? 0xFF : 0x00;
    r = h;
    do {
      const uint8_t* row = s;
      uint16_t a = di;
      for (c = 0; c < w; c++, a++, s += 3) {
        for (p = 0; p < 3; p++) vga_plane[p][a] = s[p];
        P3[a] = fill;
      }
      if (mode & 0x8000) {
        s = row;
        a = (uint16_t)(di + 0x50);
        for (c = 0; c < w; c++, a++, s += 3) {
          for (p = 0; p < 3; p++) vga_plane[p][a] = s[p];
          P3[a] = fill;
        }
        di = (uint16_t)(di + 0x50);
      }
      di = (uint16_t)(di + 0x50);
    } while (--r);
    return 0;
  }
  if (m <= 3) {
    static const uint8_t fn[4] = { 0, FN_XOR, FN_AND, FN_OR };
    r = h;
    do {
      uint16_t a = di;
      for (c = 0; c < w; c++, a++, s += 3)
        for (p = 0; p < 3; p++) vga_plane[p][a] = logic(fn[m], s[p], vga_plane[p][a]);
      di = (uint16_t)(di + 0x50);
    } while (--r);
    return 0;
  }
  r = h;
  do {
    uint16_t a = di;
    for (c = 0; c < w; c++, a++, s += 3) {
      uint8_t mask = (uint8_t)(s[0] | s[1] | s[2]);
      for (p = 0; p < 3; p++) vga_plane[p][a] = (uint8_t)((s[p] & mask) | (vga_plane[p][a] & ~mask));
      P3[a] = (uint8_t)(P3[a] & ~mask);
    }
    di = (uint16_t)(di + 0x50);
  } while (--r);
  return 0;
}

/* 27C2: blit3 with a near source. */
int16_t koei_27c2(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode, const uint8_t* data) {
  return blit3(x, y, w, h, mode, data);
}

/* 27CA: blit3 with a far source. */
int16_t opeu_blit_far(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode, const uint8_t* data) {
  return blit3(x, y, w, h, mode, data);
}

/* ---- 27DF one-bit bitmap, byte aligned ------------------------------------ */

/* Source: w bytes per row, one bit per pixel (bit 7 leftmost).  mode: low
 * byte = colour; (mode & 7FFF) >> 8 = operation.  Operation 0: opaque, each
 * byte is first cleared in all planes, then the planes of the colour receive
 * the source byte (pixel = bit ? colour : 0).  Operation 4 is 0 through the
 * pixel writer (set colour where bit = 1, rest unchanged); 1-3 XOR/AND/OR
 * through the pixel writer; 5..7F leave the writer's previous selection
 * active (24EB ignores them).  Bit 15 draws every row twice.
 *
 * On a failed clip the original returns through a corrupted stack (loc_283B
 * pops BP and RETs, consuming the caller's return address: a crash), so the
 * callers must never pass rectangles outside the screen; here it returns -1. */
static int16_t draw1(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode, const uint8_t* s) {
  uint16_t di, c, r;
  uint8_t op = (uint8_t)((mode & 0x7FFF) >> 8), color = (uint8_t)mode;
  if (!clip_and_address(x, y, w, h, &di)) return -1;
  vga_dirty = 1;
  if (op == 0) {
    r = h;
    do {
      int pass;
      const uint8_t* row = s;
      for (pass = 0; pass < ((mode & 0x8000) ? 2 : 1); pass++) {
        uint16_t a = di;
        int p;
        if (pass) {
          s = row;
          di = (uint16_t)(di + 0x50);
          a = di;
        }
        for (c = 0; c < w; c++, a++, s++)
          for (p = 0; p < 4; p++) vga_plane[p][a] = (color & (1u << p)) ? *s : 0;
      }
      di = (uint16_t)(di + 0x50);
    } while (--r);
    return 0;
  }
  pixel_select(op == 4 ? 0 : op, color);
  r = h;
  do {
    int pass;
    const uint8_t* row = s;
    for (pass = 0; pass < ((mode & 0x8000) ? 2 : 1); pass++) {
      uint16_t a = di;
      if (pass) {
        s = row;
        di = (uint16_t)(di + 0x50);
        a = di;
      }
      for (c = 0; c < w; c++, a++, s++) pixel_write(a, *s);
    }
    di = (uint16_t)(di + 0x50);
  } while (--r);
  return 0;
}

/* 28A8 (also $@opeu_code_0000_28a8, glyph routine 0 of textout.c): draw1
 * with a near source. */
int16_t opeu_draw_near(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode, const uint8_t* data) {
  return draw1(x, y, w, h, mode, data);
}

/* 28B0: draw1 with a far source. */
int16_t opeu_draw_far(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode, const uint8_t* data) {
  return draw1(x, y, w, h, mode, data);
}

/* 28C5 ($@opeu_code_0000_28c5, glyph routine 1 of textout.c: opaque glyph on
 * a coloured paper).  mode low byte DL, high byte DH.  For each plane p the
 * self-modified code at 2962..2991 writes (DH bit p ? FF : 00) OR src when
 * DL bit p is set, else (DH bit p ? FF : 00) AND ~src; so a pixel becomes
 * DL & 15 where the source bit is 1 and DH & 15 where it is 0.  Byte
 * aligned, w bytes per row, no row doubling.  No return value. */
void opeu_code_0000_28c5(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode, const uint8_t* s) {
  uint16_t di, c, r;
  uint8_t dl = (uint8_t)mode, dh = (uint8_t)(mode >> 8);
  int p;
  if (!clip_and_address(x, y, w, h, &di)) return;
  vga_dirty = 1;
  r = h;
  do {
    uint16_t a = di;
    for (c = 0; c < w; c++, a++, s++)
      for (p = 0; p < 4; p++) {
        uint8_t base = (dh & (1u << p)) ? 0xFF : 0x00;
        vga_plane[p][a] = (dl & (1u << p)) ? (uint8_t)(base | *s) : (uint8_t)(base & ~*s);
      }
    di = (uint16_t)(di + 0x50);
  } while (--r);
}

/* ---- 29A6 one-bit bitmap at any pixel x ----------------------------------- */

/* x, w in pixels (x <= 639, x + w <= 640), y, h lines (<= 400).  Source rows
 * are ceil(w / 8) bytes.  Each output byte is the source shifted right by
 * x & 7 through a 16-bit SHR, written by the pixel writer; the bits shifted
 * in at the row edges come from a fill byte that is FF for the AND operation
 * (leaving pixels unchanged) and 00 otherwise.  Whole bytes are drawn: the
 * unused low bits of the last source byte are drawn too.  mode: low byte
 * colour, high byte operation 0 set, 1 XOR, 2 AND, 3 OR, 4 = 0, others keep
 * the previous pixel-writer selection.  Returns 0 or -1. */
static int16_t draw1_shift(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode, const uint8_t* s) {
  uint16_t di, r;
  uint8_t extra, cl, op = (uint8_t)(mode >> 8), fill;
  if (x > 0x27F || w == 0 || (uint32_t)x + w > 0x280) return -1;
  extra = (uint8_t)((uint16_t)(w - 1) >> 3);
  if (y > 0x18F || h == 0 || (uint32_t)y + h > 0x190) return -1;
  di = (uint16_t)(line_addr(y) + (x >> 3));
  cl = (uint8_t)(x & 7);
  if (op == 4) op = 0;
  fill = op == 2 ? 0xFF : 0x00;
  pixel_select(op, (uint8_t)mode);
  vga_dirty = 1;
  r = h;
  do {
    uint16_t a = di;
    uint8_t i;
    pixel_write(a++, (uint8_t)((uint16_t)((fill << 8) | s[0]) >> cl));
    for (i = 0; i < extra; i++) pixel_write(a++, (uint8_t)((uint16_t)((s[i] << 8) | s[i + 1]) >> cl));
    if (cl) pixel_write(a, (uint8_t)((uint16_t)((s[extra] << 8) | fill) >> cl));
    s += extra + 1;
    di = (uint16_t)(di + 0x50);
  } while (--r);
  return 0;
}

/* 2A81: draw1_shift with a near (DS) source. */
int16_t opeu_vga_glyph(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode, const uint8_t* data) {
  return draw1_shift(x, y, w, h, mode, data);
}

/* 2A96 (also _opeu_bitmap_draw): draw1_shift with a far source. */
int16_t opeu_gfx_blit(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode, const uint8_t* data) {
  return draw1_shift(x, y, w, h, mode, data);
}

/* ---- fills ----------------------------------------------------------------- */

/* 2AAC: clear the 480 visible lines (0x4B00 words from A000:0) to colour 0
 * (write mode 2, all planes).  Leaves write mode 2 programmed.  AX = 0. */
int16_t opeu_vga_clear(void) {
  int p;
  for (p = 0; p < 4; p++) memset(vga_plane[p], 0, 0x9600);
  vga_dirty = 1;
  return 0;
}

/* 2AD2 (also _opeu_vga_fill_rect): solid rectangle, x and w in bytes, colour
 * = low byte (write mode 2, map mask F).  Leaves write mode 2 programmed.
 * Returns 0, or -1 when the clip fails. */
int16_t opeu_ui_box(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
  uint16_t di, c, r;
  int p;
  if (!clip_and_address(x, y, w, h, &di)) return -1;
  vga_dirty = 1;
  r = h;
  do {
    for (c = 0; c < w; c++)
      for (p = 0; p < 4; p++) vga_plane[p][(uint16_t)(di + c)] = (color & (1u << p)) ? 0xFF : 0x00;
    di = (uint16_t)(di + 0x50);
  } while (--r);
  return 0;
}

/* 2B0E: XOR/AND/OR the colour (low byte) over a byte-aligned rectangle via
 * the pixel writer with all bits set; operation = high byte 1..3.  Any other
 * operation, or a failed clip, returns -1 without drawing.  AND with all
 * bits set changes nothing (planes outside the colour are not touched). */
int16_t opeu_ui_highlight(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
  uint16_t di, c, r;
  uint8_t op = (uint8_t)(color >> 8);
  if (!clip_and_address(x, y, w, h, &di)) return -1;
  if (op == 0 || op >= 4) return -1;
  pixel_select(op, (uint8_t)color);
  vga_dirty = 1;
  r = h;
  do {
    for (c = 0; c < w; c++) pixel_write((uint16_t)(di + c), 0xFF);
    di = (uint16_t)(di + 0x50);
  } while (--r);
  return 0;
}

/* ---- 2B50..2D27 pixel plot and lines --------------------------------------- */

static int16_t line_error;      /* DGROUP 3EAA: +1 / -1 minor-axis step */
static uint8_t pattern_bit;     /* 3EAC: next bit of the dash pattern */
static uint8_t pattern_on;      /* 3EAD */

/* 2BA5: plot (x, y) in game coordinates (x < 640, y < 400, unsigned).
 * word: low nibble colour, bits 4-7 operation, high byte dash pattern.
 * With a pattern, bit (0x80 >> pattern_bit) decides the pixel; pattern_bit
 * advances mod 8 for every plotted point (inside the screen); a 0 bit clears
 * the colour nibble (keeping the operation) and pattern_on.  Operations, with
 * write mode 2 and the bit mask on the pixel: 0 colour, 1 XOR, 2 AND, 3 OR,
 * 4 = map mask colour, replace with colour (pixel |= colour), only while
 * pattern_on; 5..15 behave as 0. */
static void plot(uint16_t x, uint16_t y, uint16_t word) {
  uint16_t a;
  uint8_t mask, cl = (uint8_t)word, ch = (uint8_t)(word >> 8), op;
  int p;
  if (y >= 0x190 || x >= 0x280) return;
  a = (uint16_t)(line_addr(y) + (x >> 3));
  mask = (uint8_t)(0x80 >> (x & 7));
  pattern_on = 1;
  if (ch) {
    uint8_t bit = (uint8_t)(0x80 >> pattern_bit);
    pattern_bit = (uint8_t)((pattern_bit + 1) & 7);
    if (!(bit & ch)) {
      cl &= 0xF0;
      pattern_on = 0;
    }
  }
  op = (uint8_t)(cl >> 4);
  if (op >= 5) op = 0;
  vga_dirty = 1;
  if (op == 4) {
    if (!pattern_on) return;
    for (p = 0; p < 4; p++)
      if (cl & (1u << p)) vga_plane[p][a] |= mask;
    return;
  }
  {
    static const uint8_t fn[4] = { FN_REPLACE, FN_XOR, FN_AND, FN_OR };
    for (p = 0; p < 4; p++) {
      uint8_t data = (cl & (1u << p)) ? 0xFF : 0x00;
      uint8_t v = logic(fn[op], data, vga_plane[p][a]);
      vga_plane[p][a] = (uint8_t)((v & mask) | (vga_plane[p][a] & ~mask));
    }
  }
}

/* 2CE4: plot one pixel; colour word = low byte only (no pattern). */
void koei_2ce4(uint16_t x, uint16_t y, uint16_t color) {
  pattern_bit = 0;
  pattern_on = 0;
  line_error = 0;
  plot(x, y, (uint16_t)(color & 0xFF));
}

static uint16_t abs16(uint16_t v) {
  return (int16_t)v < 0 ? (uint16_t)-v : v;
}

/* 2C4E via 2D16: line from (x1, y1) to (x2, y2), colour word as plot().
 * Bresenham with the error term in DI starting at half the major length;
 * the start point is the end with the smaller major coordinate (unsigned
 * compare); the minor step 2C25 is +1 when x and y increase together (signed
 * compares), else -1.  The dash pattern restarts at bit 0. */
void opeu_ui_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
  uint16_t dx = abs16((uint16_t)(x2 - x1));
  uint16_t dy = abs16((uint16_t)(y2 - y1));
  uint16_t x, y, err, count;
  pattern_bit = 0;
  if (((int16_t)x1 < (int16_t)x2 && (int16_t)y2 > (int16_t)y1)
      || ((int16_t)x1 > (int16_t)x2 && (int16_t)y2 < (int16_t)y1))
    line_error = 1;
  else
    line_error = -1;
  if (dx >= dy) {
    y = y1;
    x = x1;
    if (!(x1 < x2)) {
      y = y2;
      x = x2;
    }
    err = (uint16_t)(dx >> 1);
    count = (uint16_t)(dx + 1);
    do {
      plot(x, y, color);
      if (err < dy) {
        err = (uint16_t)(err - dy + dx);
        y = (uint16_t)(y + line_error);
      } else {
        err = (uint16_t)(err - dy);
      }
      x++;
    } while (--count);
  } else {
    y = y1;
    x = x1;
    if (!(y1 < y2)) {
      y = y2;
      x = x2;
    }
    err = (uint16_t)(dy >> 1);
    count = (uint16_t)(dy + 1);
    do {
      plot(x, y, color);
      if (err < dx) {
        err = (uint16_t)(err - dx + dy);
        x = (uint16_t)(x + line_error);
      } else {
        err = (uint16_t)(err - dx);
      }
      y++;
    } while (--count);
  }
}

/* ---- 2D2A transparent planar blit at any pixel x -------------------------- */

/* Source as blit4 (4 plane bytes per column, w columns per row).  No clip.
 * Every output byte is written under the bit mask = OR of the plane bytes
 * shifted by x & 7, i.e. zero pixels are transparent and keep the latches.
 * Hardware quirk kept: the latches are loaded only for the first byte of a
 * row (2D7C) and for the final partial byte (2E0E); the whole bytes in
 * between (loop 2DA5) are written with the latches of the row's FIRST byte,
 * so their transparent pixels receive that byte's original pixels, not their
 * own.  w = 0 runs the middle loop 65535 times (16-bit DEC); h = 0 runs
 * 65536 rows. */
static void blit4_shift(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t* s) {
  uint16_t di = (uint16_t)(line_addr(y) + (x >> 3)), r = h;
  uint8_t cl = (uint8_t)(x & 7);
  int p;
  vga_dirty = 1;
  do {
    uint16_t a = di, n = w;
    uint8_t latch[4], mask;
    for (p = 0; p < 4; p++) latch[p] = vga_plane[p][a];
    mask = (uint8_t)((s[0] | s[1] | s[2] | s[3]) >> cl);
    for (p = 0; p < 4; p++) vga_plane[p][a] = (uint8_t)(((s[p] >> cl) & mask) | (latch[p] & ~mask));
    a++;
    s += 4;
    while (--n) {
      uint16_t prev = (uint16_t)(s[-1] | s[-2] | s[-3] | s[-4]);
      uint16_t cur = (uint16_t)(s[0] | s[1] | s[2] | s[3]);
      mask = (uint8_t)(((prev << 8) | cur) >> cl);
      for (p = 0; p < 4; p++) {
        uint8_t v = (uint8_t)((uint16_t)((s[p - 4] << 8) | s[p]) >> cl);
        vga_plane[p][a] = (uint8_t)((v & mask) | (latch[p] & ~mask));
      }
      a++;
      s += 4;
    }
    if (cl) {
      const uint8_t* l = s - 4;
      for (p = 0; p < 4; p++) latch[p] = vga_plane[p][a];
      mask = (uint8_t)((uint16_t)((l[0] | l[1] | l[2] | l[3]) << 8) >> cl);
      for (p = 0; p < 4; p++) {
        uint8_t v = (uint8_t)((uint16_t)(l[p] << 8) >> cl);
        vga_plane[p][a] = (uint8_t)((v & mask) | (latch[p] & ~mask));
      }
    }
    di = (uint16_t)(di + 0x50);
  } while (--r);
}

/* 2E49: near source. */
void koei_2e49(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t* data) {
  blit4_shift(x, y, w, h, data);
}

/* 2E57: far source. */
void koei_2e57(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t* data) {
  blit4_shift(x, y, w, h, data);
}

/* ---- 2E72 / 2ED8 save a rectangle ----------------------------------------- */

/* Copy the byte-aligned rectangle (x, w bytes) to `d` in the blit4 format,
 * reading each plane through the GC read map.  No clip.  The original's
 * LOOPs make w = 0 or h = 0 mean 65536 (overrunning the buffer); here they
 * copy nothing. */
static void save4(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t* d) {
  uint16_t si = (uint16_t)(line_addr(y) + x), r, c;
  int p;
  if (!w || !h) return;
  for (r = 0; r < h; r++, si = (uint16_t)(si + 0x50))
    for (c = 0; c < w; c++)
      for (p = 0; p < 4; p++) *d++ = vga_plane[p][(uint16_t)(si + c)];
}

/* 2E72: destination in DS. */
void koei_2e72(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t* dest) {
  save4(x, y, w, h, dest);
}

/* 2ED8: far destination. */
void opeu_bitmap_save(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t* dest) {
  save4(x, y, w, h, dest);
}

/* ---- 2F3D / 2FA5 pixel rectangle fill ------------------------------------- */

/* 2FA5: fill the pixel rectangle x1..x2, y1..y2 (inclusive) with the colour
 * (low byte) using set/reset on all planes and the edge bit masks from 2F3D.
 * No clip; everything is 16-bit, so x2 < x1 or y2 < y1 wrap round VRAM like
 * the original.  When both edges are in one byte the right-hand write uses
 * bit mask 0 one byte further (no change).  Leaves enable set/reset = F. */
void opeu_ui_fill(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
  uint16_t di = (uint16_t)((uint16_t)(y1 + VGA_TOP) * 80u + (x1 >> 3));
  uint8_t bl = (uint8_t)(0xFF >> (x1 & 7));
  uint8_t bh = (uint8_t)~(0xFF >> ((x2 & 7) + 1));
  uint16_t cx = (uint16_t)((x2 >> 3) - (x1 >> 3));
  uint16_t rows = (uint16_t)(y2 - y1 + 1);
  uint8_t planes[4];
  int p;
  for (p = 0; p < 4; p++) planes[p] = (color & (1u << p)) ? 0xFF : 0x00;
  if (cx == 0) {
    bl &= bh;
    bh = 0;
    cx = 1;
  }
  cx--;
  vga_dirty = 1;
  do {
    uint16_t a = di, i;
    for (p = 0; p < 4; p++) vga_plane[p][a] = (uint8_t)((planes[p] & bl) | (vga_plane[p][a] & ~bl));
    a++;
    for (i = 0; i < cx; i++, a++)
      for (p = 0; p < 4; p++) vga_plane[p][a] = planes[p];
    for (p = 0; p < 4; p++) vga_plane[p][a] = (uint8_t)((planes[p] & bh) | (vga_plane[p][a] & ~bh));
    di = (uint16_t)(di + 0x50);
  } while (--rows);
}

/* ---- modes ---------------------------------------------------------------- */

static uint8_t saved_mode;      /* DGROUP 3EB4 */

static void load_mode12_defaults(void) {
  static const uint8_t ega[16] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
                                   0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F };
  int i;
  memcpy(vga_attr, ega, sizeof vga_attr);
  memset(vga_dac, 0, sizeof vga_dac);
  for (i = 0; i < 64; i++) {     /* EGA rgbRGB: b g r (2/3), B G R (1/3) */
    vga_dac[i * 3 + 0] = (uint8_t)(((i >> 2) & 1) * 0x2A + ((i >> 5) & 1) * 0x15);
    vga_dac[i * 3 + 1] = (uint8_t)(((i >> 1) & 1) * 0x2A + ((i >> 4) & 1) * 0x15);
    vga_dac[i * 3 + 2] = (uint8_t)(((i >> 0) & 1) * 0x2A + ((i >> 3) & 1) * 0x15);
  }
  vga_pel_mask = 0xFF;
}

/* INT 10h AH=00h: set a BIOS mode. Mode 12h clears VRAM and loads the
 * default attribute palette and DAC; any other mode is shown as text (the
 * port has no text screen: it renders blank). */
static void bios_set_mode(uint8_t mode) {
  int p;
  for (p = 0; p < 4; p++) memset(vga_plane[p], 0, VGA_PLANE_SIZE);
  load_mode12_defaults();
  vga_text_mode = (mode & 0x7F) != 0x12;
  vga_dirty = 1;
}

/* 2FEA: remember the current BIOS mode (AH=0Fh, & 7F) at DGROUP 3EB4 and
 * set mode 12h (cleared VRAM, default attribute palette and DAC), hide the
 * cursor (AH=01h CX=2000h), then INT 16h AX=1300h DL=0 (keyboard; no
 * display effect). */
void koei_2fea(void) {
  saved_mode = vga_text_mode ? 0x03 : 0x12;
  bios_set_mode(0x12);
}

/* 300C: set the remembered mode again and the cursor 0607h. MAIN runs after
 * the launcher has set mode 12h, so the remembered mode is 12h and this
 * clears the graphics screen; the launcher restores the text mode. */
void opeu_text_mode(void) {
  bios_set_mode(saved_mode);
}

/* ---- 301F screen copy ------------------------------------------------------ */

/* 301F (also _opeu_vga_copy): copy a byte-aligned rectangle within VRAM with
 * write mode 1 (all four planes through the latches).  Source (x1, y1),
 * destination (x2, y2), w bytes, h rows; no clip.  When the destination is
 * below the source (DI >= SI, unsigned) the copy runs backwards from the
 * last byte (STD), so overlapping copies behave like memmove per byte in
 * the original's order.  h = 0 copies 65536 rows (LOOP).  Leaves write
 * mode 1 programmed. */
void opeu_screen_copy(uint16_t x1, uint16_t y1, uint16_t w, uint16_t h, uint16_t x2, uint16_t y2) {
  uint16_t si = (uint16_t)((uint16_t)(y1 + VGA_TOP) * 0x50u + x1);
  uint16_t di = (uint16_t)((uint16_t)(y2 + VGA_TOP) * 0x50u + x2);
  uint16_t step = (uint16_t)(0x50 - w), rows = h, c;
  int back = 0, p;
  if (!(di < si)) {
    uint16_t off = (uint16_t)(0x50u * (uint16_t)(h - 1) + w - 1);
    si = (uint16_t)(si + off);
    di = (uint16_t)(di + off);
    back = 1;
    step = (uint16_t)(w - 0x50);
  }
  vga_dirty = 1;
  do {
    for (c = 0; c < w; c++) {
      for (p = 0; p < 4; p++) vga_plane[p][di] = vga_plane[p][si];
      if (back) {
        si--;
        di--;
      } else {
        si++;
        di++;
      }
    }
    si = (uint16_t)(si + step);
    di = (uint16_t)(di + step);
  } while (--rows);
}

/* ---- 30A6 picture decoder -------------------------------------------------- */

/* Pictures are coded in units of 4 pixels: unit u is nibble u & 1 (0 = high,
 * pixels 0-3; 1 = low, pixels 4-7) of VRAM byte (u >> 1) + 0C80h (line 40),
 * 160 units per line, and a unit value is 4 plane nibbles.  Stream:
 *   width  (word, 1..640), height (word, 1..400), x + width <= 640,
 *   y + height <= 400, y <= 399, x <= 639 (x is truncated to a unit);
 *   [masked only] 8 words: the run-colour table 3096;
 *   then per row, commands until (width + 3) / 4 units are covered (the last
 *   command may run past the row end; the next row starts at the line start):
 *   1sxxcccc  copy c+1 units from xx+1 units back (s = 0) or from xx+1 rows
 *             up (s = 1), unit by unit (overlap repeats, LZ style).  An odd
 *             nibble distance is done with data rotate 4; same parity with
 *             nibble bit masks; both give a plain nibble copy.
 *   0.......  literal, decoded by the handler:
 *     plain  (3320): 0nnnpppp qqqqqqqq -> n+1 units of value p:q, i.e.
 *            plane 2 = p, plane 1 = q >> 4, plane 0 = q & 15, plane 3 = 0;
 *     masked (334E): 0kkkkrrr, count r+1, then by kkkk (bits 3-6):
 *            0000 -> value 0; 1kkk -> run table entry kkk;
 *            0111 -> two bytes a b: value a:b (all four planes);
 *            0001..0110 -> one byte b, mode m = kkkk - 1:
 *              0: planes 1,0 = b   1: planes 2,0   2: planes 3,0
 *              3: planes 2,1       4: planes 3,1   5: planes 3,2
 *            (high nibble of b to the higher plane, the others 0).
 * Value word v = n3 n2 n1 n0 (nibbles, n0 lowest): plane p <- n_p.
 * Returns 0, or -1 for a bad header (the stream is then partly consumed). */
static uint16_t decode_runs[8];     /* CS 3096, persists between pictures */

static void unit_put(uint16_t u, uint16_t v) {
  uint16_t a = (uint16_t)((u >> 1) + 0x0C80);
  uint8_t keep = (u & 1) ? 0xF0 : 0x0F;
  int p;
  for (p = 0; p < 4; p++) {
    uint8_t n = (uint8_t)((v >> (p * 4)) & 0x0F);
    uint8_t bits = (u & 1) ? n : (uint8_t)(n << 4);
    vga_plane[p][a] = (uint8_t)((vga_plane[p][a] & keep) | bits);
  }
}

static uint16_t unit_get(uint16_t u) {
  uint16_t a = (uint16_t)((u >> 1) + 0x0C80), v = 0;
  int p;
  for (p = 0; p < 4; p++) {
    uint8_t b = vga_plane[p][a];
    v |= (uint16_t)(((u & 1) ? (b & 0x0F) : (b >> 4)) << (p * 4));
  }
  return v;
}

static int16_t decode(uint16_t x, uint16_t y, VgaByteReader rd, int masked) {
  uint16_t width, height, di, units, rows;
  width = rd();
  width = (uint16_t)(width | (rd() << 8));
  if ((uint16_t)(width - 1) > 0x27F) return -1;
  height = rd();
  height = (uint16_t)(height | (rd() << 8));
  if ((uint16_t)(height - 1) > 0x18F) return -1;
  if (y > 0x18F || (uint16_t)(height + y) > 0x190) return -1;
  if (x > 0x27F || (uint16_t)(x + width) > 0x280) return -1;
  di = (uint16_t)((x >> 2) + y * 160u);
  units = (uint16_t)((width + 3) >> 2);
  if (masked) {
    int i;
    for (i = 0; i < 8; i++) {
      uint16_t lo = rd();
      decode_runs[i] = (uint16_t)(lo | (rd() << 8));
    }
  }
  vga_dirty = 1;
  rows = height;
  do {
    uint16_t d = di;
    uint16_t left = units;
    for (;;) {
      uint8_t b = rd();
      uint16_t n, i;
      if (!(b & 0x80)) {
        uint16_t v;
        if (!masked) {
          n = (uint16_t)(((b >> 4) & 7) + 1);
          v = (uint16_t)(((b & 0x0F) << 8) | rd());
        } else {
          uint8_t k = (uint8_t)(b & 0x78);
          n = (uint16_t)((b & 7) + 1);
          if (k == 0)
            v = 0;
          else if (k & 0x40)
            v = decode_runs[(k >> 3) & 7];
          else if (k == 0x38) {
            uint16_t hi = rd();
            v = (uint16_t)((hi << 8) | rd());
          } else {
            uint8_t c = rd();
            switch ((k - 8) >> 3) {
            case 0: v = c; break;
            case 1: v = (uint16_t)(((c & 0xF0) << 4) | (c & 0x0F)); break;
            case 2: v = (uint16_t)(((c & 0xF0) << 8) | (c & 0x0F)); break;
            case 3: v = (uint16_t)(c << 4); break;
            case 4: v = (uint16_t)(((c & 0xF0) << 8) | ((c & 0x0F) << 4)); break;
            default: v = (uint16_t)(c << 8); break;
            }
          }
        }
        for (i = 0; i < n; i++) unit_put((uint16_t)(d + i), v);
      } else {
        uint16_t back = (uint16_t)(((b & 0x30) >> 4) + 1);
        uint16_t s;
        if (b & 0x40) back = (uint16_t)(back * 160u);
        n = (uint16_t)((b & 0x0F) + 1);
        s = (uint16_t)(d - back);
        for (i = 0; i < n; i++) unit_put((uint16_t)(d + i), unit_get((uint16_t)(s + i)));
      }
      d = (uint16_t)(d + n);
      if (left <= n) break;
      left = (uint16_t)(left - n);
    }
    di = (uint16_t)(di + 160);
  } while (--rows);
  return 0;
}

/* 3490: masked picture (run table, 16-colour literals). */
int16_t opeu_picture_decode_masked(uint16_t x, uint16_t y, VgaByteReader reader) {
  return decode(x, y, reader, 1);
}

/* 34A4: plain picture (8-colour literals, plane 3 = 0). */
int16_t opeu_picture_decode(uint16_t x, uint16_t y, VgaByteReader reader) {
  return decode(x, y, reader, 0);
}

/* ---- host side -------------------------------------------------------------- */

/* The display state MAIN starts in: the launcher (KOEI.COM, src/koei/
 * launcher.asm) sets BIOS mode 12h before it runs FMDRV, OPEN.EXE and MAIN,
 * so MAIN's first screen (the input-device prompt, drawn before 2FEA) is
 * already in mode 12h with the BIOS default palette. */
void vgalib_init(void) {
  pixel_color = 0x07;
  pixel_function = 0;
  pixel_mask_mode = 0;
  line_error = 0;
  pattern_bit = pattern_on = 0;
  memset(decode_runs, 0, sizeof decode_runs);
  saved_mode = 0x03;
  bios_set_mode(0x12);
}

int vgalib_render(uint8_t* out, uint8_t pal[768]) {
  static uint64_t lut[256];
  static int lut_ready;
  uint8_t cmap[16];
  int changed = vga_dirty, i, y, x;
  if (!lut_ready) {
    for (i = 0; i < 256; i++) {
      uint64_t v = 0;
      int b;
      for (b = 0; b < 8; b++)
        if (i & (0x80 >> b)) v |= (uint64_t)1 << (b * 8);
      lut[i] = v;
    }
    lut_ready = 1;
  }
  vga_dirty = 0;
  for (i = 0; i < 768; i++) {
    uint8_t v = (uint8_t)(vga_dac[i] & 0x3F);
    pal[i] = (uint8_t)((v << 2) | (v >> 4));
  }
  if (vga_text_mode) {
    memset(out, 0, 640 * 480);
    return changed;
  }
  for (i = 0; i < 16; i++) cmap[i] = (uint8_t)(vga_attr[i] & 0x3F & vga_pel_mask);
  for (y = 0; y < 480; y++) {
    uint16_t a = (uint16_t)(y * 80);
    uint8_t* o = out + y * 640;
    for (x = 0; x < 80; x++, a++, o += 8) {
      uint64_t px = lut[P0[a]] | (lut[P1[a]] << 1) | (lut[P2[a]] << 2) | (lut[P3[a]] << 3);
      for (i = 0; i < 8; i++) o[i] = cmap[(px >> (i * 8)) & 0x0F];
    }
  }
  return changed;
}
