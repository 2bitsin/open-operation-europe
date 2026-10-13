// Checks of the drawing library on the canvas against results worked out by hand.
#include "vgalib.hpp"

#include "canvas.hpp"
#include <format>
#include <print>
#include <source_location>
#include <stdio.h>
#include <string.h>
#include <type_traits>
#include <utility>

static int failures, checks;

namespace {

// The message of a failed check, read at compile time, and where the check is.
template <typename... _Args>
struct CheckMessage {
  std::format_string<_Args...> text;
  std::source_location         where;

  template <typename _Text>
  consteval CheckMessage(_Text const& text, std::source_location where = std::source_location::current()) : text{ text }, where{ where } { }
};

// Count the check; print the place and the message when it does not hold.
template <typename... _Args>
auto Check(bool held, CheckMessage<std::type_identity_t<_Args>...> message, _Args&&... args) -> void {
  checks++;
  if (!held) {
    failures++;
    std::println("FAIL {}:{}: {}", message.where.file_name(), message.where.line(), std::format(message.text, std::forward<_Args>(args)...));
  }
}

}

/* Pixel colour at game coordinates (x, y) (game line y = screen line y + 40). */
static int px(int x, int y) {
  return opeu::services::Screen().Pixel(opeu::services::GameAt(x, y));
}

static void Setpx(int x, int y, int c) {
  opeu::services::Screen().Put(opeu::services::GameAt(x, y), (uint8_t)c);
}

static void Fillpx(int c) {
  for (int i = 0; i < 640 * 480; i++) opeu::services::Screen().Put(i, (uint8_t)c);
}

/* ---- picture stream reader ---- */
static const uint8_t* stream;
static uint8_t        Rd(void) {
  return *stream++;
}

static void TestBlit4(void) {
  /* 2 columns x 2 rows; column bytes are planes 0..3. */
  static const uint8_t IMG[16] = {
    0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, /* row 0: colour 1, colour 2 */
    0xF0, 0xF0, 0xF0, 0xF0, 0x0F, 0x00, 0x00, 0x0F  /* row 1: 15 x4 + 0, 0 x4 + 9 */
  };
  uint8_t back[16];
  int     x;
  Fillpx(0);
  Check(opeu_bitmap_restore(10, 5, 2, 2, 0, IMG) == 0, "restore ok");
  for (x = 0; x < 8; x++) Check(px(80 + x, 5) == 1, "row0 col0 x={} c={}", x, px(80 + x, 5));
  for (x = 8; x < 16; x++) Check(px(80 + x, 5) == 2, "row0 col1");
  Check(px(80, 6) == 15 && px(84, 6) == 0, "row1 col0 nibbles");
  Check(px(88, 6) == 0 && px(92, 6) == 9, "row1 col1 nibbles");
  opeu_bitmap_save(10, 5, 2, 2, back);
  Check(memcmp(back, IMG, 16) == 0, "save round trip");

  /* clip failures */
  Check(opeu_bitmap_restore(79, 0, 2, 1, 0, IMG) == -1, "x+w > 80");
  Check(opeu_bitmap_restore(0, 399, 1, 2, 0, IMG) == -1, "y+h > 400");
  Check(opeu_bitmap_restore(0, 0, 1, 1, 5, IMG) == -1, "mode 5");

  /* row doubling (bit 15): 2 source rows -> 4 lines */
  Fillpx(0);
  opeu_bitmap_restore(0, 0, 1, 2, 0x8000, IMG);
  /* w = 1: source rows are img[0..3] (colour 1) and img[4..7] (colour 2) */
  Check(px(0, 0) == 1 && px(0, 1) == 1 && px(0, 2) == 2 && px(0, 3) == 2 && px(0, 4) == 0, "doubled rows");

  /* transparent mode 4: zero pixels keep the background */
  Fillpx(6);
  opeu_bitmap_restore(0, 0, 2, 2, 4, IMG);
  Check(px(0, 1) == 15 && px(4, 1) == 6, "mode4 opaque/transparent");
  Check(px(8, 1) == 6 && px(12, 1) == 9, "mode4 second column");

  /* XOR mode */
  Fillpx(3);
  koei_266c(0, 0, 1, 1, 1, IMG);
  Check(px(0, 0) == (3 ^ 1), "xor");
  /* AND mode / OR mode */
  Fillpx(3);
  koei_266c(0, 0, 1, 1, 2, IMG + 4); /* colour 2 */
  Check(px(0, 0) == (3 & 2), "and");
  Fillpx(4);
  koei_266c(0, 0, 1, 1, 3, IMG + 4);
  Check(px(0, 0) == (4 | 2), "or");
}

static void TestBlit3(void) {
  static const uint8_t IMG[3] = { 0xFF, 0x0F, 0x00 };
  Fillpx(8);
  opeu_blit_far(0, 0, 1, 1, 0, IMG);
  Check(px(0, 0) == 1 && px(4, 0) == 3, "blit3 plane3 cleared");
  Fillpx(0);
  koei_27c2(0, 0, 1, 1, 0x4000, IMG);
  Check(px(0, 0) == 9 && px(4, 0) == 11, "blit3 plane3 set by bit 14");
  Fillpx(12);                        /* planes 2,3 */
  opeu_blit_far(0, 0, 1, 1, 1, IMG); /* XOR planes 0-2 only */
  Check(px(0, 0) == (12 ^ 1) && px(4, 0) == (12 ^ 3), "blit3 xor keeps plane 3");
  Fillpx(14);
  {
    static const uint8_t M[3] = { 0xF0, 0x00, 0x00 };
    opeu_blit_far(0, 0, 1, 1, 4, M);
    Check(px(0, 0) == 1 && px(4, 0) == 14, "blit3 mode4 clears plane3 under mask");
  }
}

static void TestDraw1(void) {
  static const uint8_t BITS[2] = { 0xA0, 0x01 };
  Fillpx(7);
  opeu_draw_near(1, 2, 2, 1, 0x0005, BITS); /* opaque, colour 5 */
  Check(px(8, 2) == 5 && px(9, 2) == 0 && px(10, 2) == 5 && px(11, 2) == 0, "opaque");
  Check(px(23, 2) == 5 && px(22, 2) == 0, "opaque byte 2");

  Fillpx(7);
  opeu_draw_far(1, 2, 1, 1, 0x0409, BITS); /* op 4 = set, colour 9 */
  Check(px(8, 2) == 9 && px(9, 2) == 7, "transparent set");

  Fillpx(6);
  opeu_draw_far(1, 2, 1, 1, 0x0103, BITS); /* XOR 3 */
  Check(px(8, 2) == (6 ^ 3) && px(9, 2) == 6, "xor");
  Fillpx(6);
  opeu_draw_far(1, 2, 1, 1, 0x0202, BITS); /* AND: 0 bits clear colour planes */
  Check(px(8, 2) == 6 && px(9, 2) == (6 & ~2), "and");
  Fillpx(4);
  opeu_draw_far(1, 2, 1, 1, 0x0301, BITS); /* OR */
  Check(px(8, 2) == 5 && px(9, 2) == 4, "or");
  /* operation 7 keeps the previous selection (OR 1) and ignores colour 8 */
  Fillpx(4);
  opeu_draw_far(1, 2, 1, 1, 0x0708, BITS);
  Check(px(8, 2) == 5 && px(9, 2) == 4, "op 7 keeps previous writer state");
  /* doubled */
  Fillpx(0);
  opeu_draw_far(0, 0, 1, 1, 0x800F, BITS);
  Check(px(0, 0) == 15 && px(0, 1) == 15 && px(1, 1) == 0, "doubled 1-bit");

  /* 28C5: fg = DL, paper = DH */
  Fillpx(7);
  opeu_code_0000_28c5(0, 0, 1, 1, 0x030C, BITS);
  Check(px(0, 0) == 12 && px(1, 0) == 3 && px(2, 0) == 12 && px(7, 0) == 3, "28c5 two colours");
}

static void TestShiftGlyph(void) {
  /* 10 pixels wide: 2 source bytes per row; x = 3 */
  static const uint8_t G[2] = { 0xFF, 0xC0 };
  int                  x;
  Fillpx(1);
  Check(opeu_vga_glyph(3, 0, 10, 1, 0x000E, G) == 0, "glyph ok");
  for (x = 0; x < 3; x++) Check(px(x, 0) == 1, "left untouched {}", x);
  for (x = 3; x < 13; x++) Check(px(x, 0) == 14, "glyph pixel {}", x);
  for (x = 13; x < 24; x++) Check(px(x, 0) == 1, "right untouched {}", x);
  /* AND with fill FF: pixels outside the glyph keep their colour */
  Fillpx(15);
  opeu_gfx_blit(3, 0, 10, 1, 0x0204, G);
  /* AND: 1 bits and the FF fill keep pixels; the zero bits of the last
       source byte (glyph pixels 10..15 -> x 13..18) clear plane 2 */
  for (x = 0; x < 24; x++) Check(px(x, 0) == ((x >= 13 && x <= 18) ? 11 : 15), "and glyph x={} got {}", x, px(x, 0));
  Check(opeu_vga_glyph(635, 0, 10, 1, 0, G) == -1, "glyph clip");
}

static void TestFills(void) {
  int x;
  Fillpx(0);
  Check(opeu_ui_box(2, 3, 2, 2, 0x0B) == 0, "box");
  Check(px(15, 3) == 0 && px(16, 3) == 11 && px(31, 4) == 11 && px(32, 4) == 0, "box extent");
  Check(opeu_vga_fill_rect(80, 0, 1, 1, 1) == -1, "box clip");

  Fillpx(0);
  opeu_ui_fill(5, 10, 21, 11, 6);
  for (x = 0; x < 30; x++) Check(px(x, 10) == ((x >= 5 && x <= 21) ? 6 : 0), "fill x={}", x);
  Check(px(5, 12) == 0 && px(5, 11) == 6 && px(5, 9) == 0, "fill rows");
  Fillpx(0);
  opeu_ui_fill(2, 0, 4, 0, 3); /* single byte */
  for (x = 0; x < 16; x++) Check(px(x, 0) == ((x >= 2 && x <= 4) ? 3 : 0), "fill1 x={}", x);

  Fillpx(5);
  Check(opeu_ui_highlight(0, 0, 1, 1, 0x0103) == 0, "highlight xor");
  Check(px(0, 0) == 6, "highlight xor result");
  Check(opeu_ui_highlight(0, 0, 1, 1, 0x0003) == -1, "highlight op 0 rejected");
  Fillpx(5);
  opeu_ui_highlight(0, 0, 1, 1, 0x0205);
  Check(px(0, 0) == 5, "highlight AND all bits: no change");

  Fillpx(9);
  opeu_vga_clear();
  Check(px(0, -40) == 0 && px(639, 439) == 0, "clear visible");
}

static void TestLines(void) {
  int x, y;
  Fillpx(0);
  opeu_ui_line(0, 0, 10, 0, 0x0004);
  for (x = 0; x <= 11; x++) Check(px(x, 0) == (x <= 10 ? 4 : 0), "hline {}", x);
  Fillpx(0);
  opeu_ui_line(3, 9, 3, 0, 0x0002); /* vertical, reversed */
  for (y = 0; y <= 10; y++) Check(px(3, y) == (y <= 9 ? 2 : 0), "vline {}", y);
  Fillpx(0);
  opeu_ui_line(0, 0, 4, 4, 0x0001);
  for (x = 0; x <= 4; x++) Check(px(x, x) == 1, "diag {}", x);
  Fillpx(0);
  opeu_ui_line(0, 4, 4, 0, 0x0001); /* rising: step -1 */
  for (x = 0; x <= 4; x++) Check(px(x, 4 - x) == 1, "antidiag {}", x);
  /* shallow: 0,0 -> 4,2: err starts 2: x0 y0, x1 y1? (2-2=0 no borrow) */
  Fillpx(0);
  opeu_ui_line(0, 0, 4, 2, 0x0001);
  /* di=2, dy=2, dx=4: (0,0) di=0; (1,0) borrow -> di=2, y=1; (2,1) di=0;
       (3,1) borrow -> y=2; (4,2) */
  Check(px(0, 0) == 1 && px(1, 0) == 1 && px(2, 1) == 1 && px(3, 1) == 1 && px(4, 2) == 1, "bresenham sequence");
  Check(px(2, 0) == 0 && px(4, 1) == 0, "bresenham off-points");
  /* dash pattern 0xAA with mode 0: off pixels drawn in colour 0 */
  Fillpx(7);
  opeu_ui_line(0, 0, 7, 0, 0xAA03);
  for (x = 0; x < 8; x++) Check(px(x, 0) == ((x & 1) ? 0 : 3), "dash set {}", x);
  /* mode 4 (pixel |= colour) only on pattern bits */
  Fillpx(8);
  opeu_ui_line(0, 0, 7, 0, 0xF041);
  for (x = 0; x < 8; x++) Check(px(x, 0) == (x < 4 ? 9 : 8), "dash or {}", x);
  /* XOR line */
  Fillpx(5);
  opeu_ui_line(0, 0, 2, 0, 0x0013);
  Check(px(0, 0) == 6 && px(3, 0) == 5, "xor line");
  /* single pixel */
  Fillpx(0);
  Koei2ce4(639, 399, 0x0F);
  Koei2ce4(640, 0, 0x0F);
  Check(px(639, 399) == 15, "plot");
  Check(px(0, 1) == 0, "plot clipped");
}

static void TestBlit4ShiftLatchQuirk(void) {
  /* 2 source columns, shift 4.  Column 0 all colour 1 except transparent
     * right half; column 1 transparent left half. */
  static const uint8_t IMG[8] = { 0xF0, 0, 0, 0, 0x0F, 0, 0, 0 };
  int                  x;
  Fillpx(0);
  for (x = 0; x < 24; x++) Setpx(x, 0, x < 8 ? 2 : (x < 16 ? 4 : 8));
  Koei2e49(4, 0, 2, 1, IMG);
  /* byte 0 (x 0..7): mask 0F -> x4..7 = colour 1, x0..3 keep 2 */
  for (x = 0; x < 4; x++) Check(px(x, 0) == 2, "first byte keeps {}", x);
  for (x = 4; x < 8; x++) Check(px(x, 0) == 1, "first byte draws {}", x);
  /* byte 1 (x 8..15): data (F0 0F) >> 4 = 0x00 | mask (F0<<8|0F)>>4 = 0x00 ->
       all bits from the latch of byte 0 (colour 2), not its own colour 4 */
  for (x = 8; x < 16; x++) Check(px(x, 0) == 2, "middle byte gets first byte's latch {}", x);
  /* final partial byte (x 16..23): mask = 0F << 4 = F0 with fresh latch:
       x16..19 colour 1, x20..23 keep 8 */
  for (x = 16; x < 20; x++) Check(px(x, 0) == 1, "tail draws {}", x);
  for (x = 20; x < 24; x++) Check(px(x, 0) == 8, "tail keeps {}", x);
}

static void TestScreenCopy(void) {
  int x;
  Fillpx(0);
  for (x = 0; x < 24; x++) Setpx(x, 0, x / 8 + 1); /* bytes: 1, 2, 3 */
  opeu_screen_copy(0, 0, 2, 1, 1, 0);              /* overlapping, dest right */
  Check(px(0, 0) == 1 && px(8, 0) == 1 && px(16, 0) == 2, "backward copy = memmove");
  Fillpx(0);
  for (x = 0; x < 24; x++) Setpx(x, 0, x / 8 + 1);
  opeu_vga_copy(1, 0, 2, 1, 0, 0); /* overlapping, dest left */
  Check(px(0, 0) == 2 && px(8, 0) == 3 && px(16, 0) == 3, "forward copy");
  Fillpx(0);
  Setpx(0, 0, 5);
  opeu_screen_copy(0, 0, 1, 1, 3, 2);
  Check(px(24, 2) == 5, "copy to another line");
  /* rows overlapping downwards: the block moves intact (bottom row first) */
  Fillpx(0);
  Setpx(0, 0, 1);
  Setpx(0, 1, 2);
  Setpx(0, 2, 3);
  opeu_screen_copy(0, 0, 1, 3, 0, 1);
  Check(px(0, 1) == 1 && px(0, 2) == 2 && px(0, 3) == 3, "overlapping rows downwards");
  Fillpx(0);
  Setpx(0, 1, 1);
  Setpx(0, 2, 2);
  Setpx(0, 3, 3);
  opeu_screen_copy(0, 1, 1, 3, 0, 0);
  Check(px(0, 0) == 1 && px(0, 1) == 2 && px(0, 2) == 3, "overlapping rows upwards");
}

static void TestPicture(void) {
  /* plain: 8x2 picture at (8, 3) = 2 units per row.
       row 0: literal n=2 value p=0x5? -> byte 0x15 0x?? : n = ((1)&7)+1 = 2,
              plane2 = 5&? ... use b=0x14, q=0x21: units value 0x421:
              plane2=4, plane1=2, plane0=1 (bit patterns of 4 pixels).
       row 1: copy from 1 row up, 2 units: 1 1 00 0001 = 0xC1. */
  static const uint8_t PLAIN_PICTURE[] = { 8, 0, 2, 0, 0x14, 0x21, 0xC1 };
  static const uint8_t MASKED[]        = {
    4,   0, 1, 0, 0x34, 0x12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* run 0 = 0x1234 */
    0x40                                                                /* run 0, 1 unit */
  };
  static const uint8_t MASKED2[] = {
    8,   0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x38, 0xAB, 0xCD, /* 0111 000 -> 1 unit, value ABCD */
    0x80                                                                            /* copy 1 unit from 1 back */
  };
  static const uint8_t LZ[] = { 16, 0, 1, 0, 0x00, 0x0F, 0x82 }; /* 1 lit + copy 3 */
  int                  x;
  Fillpx(0);
  stream = PLAIN_PICTURE;
  Check(opeu_picture_decode(8, 3, Rd) == 0, "plain decode ok");
  /* plane0 nibble 1 = 0001: pixel 3 only; plane1 nibble 2 = 0010: pixel 2;
       plane2 nibble 4 = 0100: pixel 1 */
  for (x = 0; x < 8; x++) {
    int e = (x % 4 == 3) ? 1 : (x % 4 == 2) ? 2 : (x % 4 == 1) ? 4 : 0;
    Check(px(8 + x, 3) == e, "plain row0 x={} got {} want {}", x, px(8 + x, 3), e);
    Check(px(8 + x, 4) == e, "plain row1 copy-up x={}", x);
  }
  Check(stream == PLAIN_PICTURE + sizeof PLAIN_PICTURE, "plain stream consumed");

  Fillpx(0);
  stream = MASKED;
  Check(opeu_picture_decode_masked(4, 0, Rd) == 0, "masked decode ok");
  /* value 0x1234: plane0=4 (0100) pixel1, plane1=3 (0011) pixels 2,3,
       plane2=2 (0010) pixel 2, plane3=1 (0001) pixel 3 */
  Check(px(4, 0) == 0 && px(5, 0) == 1 && px(6, 0) == 6 && px(7, 0) == 10, "masked run table: {} {} {} {}", px(4, 0), px(5, 0), px(6, 0), px(7, 0));

  Fillpx(0);
  stream = MASKED2;
  opeu_picture_decode_masked(0, 0, Rd);
  /* ABCD: plane0=D(1101) plane1=C(1100) plane2=B(1011) plane3=A(1010) */
  {
    int e0 = 1 | 2 | 4 | 8, e1 = 1 | 2 | 0 | 0, e2 = 0 | 0 | 4 | 8, e3 = 1 | 0 | 4 | 0;
    Check(px(0, 0) == e0 && px(1, 0) == e1 && px(2, 0) == e2 && px(3, 0) == e3, "full literal {} {} {} {}", px(0, 0), px(1, 0), px(2, 0), px(3, 0));
    Check(px(4, 0) == e0 && px(5, 0) == e1 && px(6, 0) == e2 && px(7, 0) == e3, "copy 1 unit back (odd parity, rotate path)");
  }

  Fillpx(0);
  stream = LZ;
  opeu_picture_decode(0, 0, Rd);
  for (x = 0; x < 16; x++) Check(px(x, 0) == 1, "lz repeat x={}", x);

  {
    static const uint8_t BAD[] = { 0x81, 0x02 };
    stream                     = BAD;
    Check(opeu_picture_decode(0, 0, Rd) == -1, "width 641 rejected");
  }
}

static void TestPaletteRender(void) {
  static const uint8_t RGB[48] = { 1, 2, 3, 15, 0, 15 };
  static uint8_t       out[640 * 480];
  static uint8_t       pal[768];
  int                  w;
  vgalib_init();
  koei_2fea();
  koei_vga_identity_palette();
  Check(opeu_set_palette(1, 2, RGB) == 0, "set palette");
  /* entry 1: R = b1<<2 = 8, G = b2<<2 = 12, B = b0<<2 = 4 */
  auto const dac{ opeu::services::Screen().Palette() };
  Check(dac[3] == 8 && dac[4] == 12 && dac[5] == 4, "dac entry 1");
  Check(dac[6] == 0 && dac[7] == 60 && dac[8] == 60, "dac entry 2");
  opeu_set_palette(15, 2, RGB); /* runs past 15 */
  Check(dac[16 * 3] == 0 && dac[16 * 3 + 1] == 60, "dac entry 16 written");
  opeu_set_palette(4, 20, RGB); /* count -> 16-4 = 12 (reads 12 entries) */

  Setpx(0, 0, 1);
  Setpx(639, 399, 2);
  w = VgalibRender(out, pal);
  Check(w == 1, "render dirty");
  Check(out[40 * 640] == 1 && out[439 * 640 + 639] == 2 && out[0] == 0, "render pixels");
  Check(pal[3] == ((8 << 2) | (8 >> 4)) && pal[4] == ((12 << 2) | (12 >> 4)), "palette scale");
  Check(VgalibRender(out, pal) == 0, "render clean");
  opeu::services::Screen().MapColour(1, 0x3F);
  VgalibRender(out, pal);
  Check(out[40 * 640] == 0x3F, "colour map applied");
  opeu_text_mode();
  VgalibRender(out, pal);
  Check(out[40 * 640] == 0, "text mode renders blank");
}

/* Run by services.test/c-suites.cpp; returns non-zero on a failure. */
int TestVgalib(void) {
  vgalib_init();
  koei_2fea();
  TestBlit4();
  TestBlit3();
  TestDraw1();
  TestShiftGlyph();
  TestFills();
  TestLines();
  TestBlit4ShiftLatchQuirk();
  TestScreenCopy();
  TestPicture();
  TestPaletteRender();
  printf("%d checks, %d failures\n", checks, failures);
  return failures != 0;
}
