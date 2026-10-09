/* Standalone checks of vgalib.c against hand-derived results of the
 * original vgalib.asm semantics.
 *   cc -std=c99 -Wall -Wextra -O2 -I port/glue port/glue/vgalib.c port/glue/test_vgalib.c
 */
#include <stdio.h>
#include <string.h>
#include "vgalib.hpp"

static int failures, checks;

#define CHECK(cond, ...) do { checks++; if (!(cond)) { failures++; \
    printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

/* Pixel colour at game coordinates (x, y) (game line y = screen line y+40). */
static int px(int x, int y)
{
    unsigned a = (unsigned)(y + 40) * 80u + (unsigned)x / 8u;
    int b = 7 - x % 8, c = 0, p;
    for (p = 0; p < 4; p++) c |= ((vga_plane[p][a & 0xFFFF] >> b) & 1) << p;
    return c;
}

static void setpx(int x, int y, int c)
{
    unsigned a = (unsigned)(y + 40) * 80u + (unsigned)x / 8u;
    int b = 7 - x % 8, p;
    for (p = 0; p < 4; p++) {
        if (c & (1 << p)) vga_plane[p][a] |= (uint8_t)(1 << b);
        else vga_plane[p][a] &= (uint8_t)~(1 << b);
    }
}

static void fillpx(int c)
{
    int p;
    for (p = 0; p < 4; p++)
        memset(vga_plane[p], (c >> p) & 1 ? 0xFF : 0x00, VGA_PLANE_SIZE);
}

/* ---- picture stream reader ---- */
static const uint8_t *stream;
static uint8_t rd(void) { return *stream++; }

static void test_blit4(void)
{
    /* 2 columns x 2 rows; column bytes are planes 0..3. */
    static const uint8_t img[16] = {
        0xFF,0x00,0x00,0x00,  0x00,0xFF,0x00,0x00,     /* row 0: colour 1, colour 2 */
        0xF0,0xF0,0xF0,0xF0,  0x0F,0x00,0x00,0x0F      /* row 1: 15 x4 + 0, 0 x4 + 9 */
    };
    uint8_t back[16];
    int x;
    fillpx(0);
    CHECK(opeu_bitmap_restore(10, 5, 2, 2, 0, img) == 0, "restore ok");
    for (x = 0; x < 8; x++) CHECK(px(80 + x, 5) == 1, "row0 col0 x=%d c=%d", x, px(80 + x, 5));
    for (x = 8; x < 16; x++) CHECK(px(80 + x, 5) == 2, "row0 col1");
    CHECK(px(80, 6) == 15 && px(84, 6) == 0, "row1 col0 nibbles");
    CHECK(px(88, 6) == 0 && px(92, 6) == 9, "row1 col1 nibbles");
    opeu_bitmap_save(10, 5, 2, 2, back);
    CHECK(memcmp(back, img, 16) == 0, "save round trip");

    /* clip failures */
    CHECK(opeu_bitmap_restore(79, 0, 2, 1, 0, img) == -1, "x+w > 80");
    CHECK(opeu_bitmap_restore(0, 399, 1, 2, 0, img) == -1, "y+h > 400");
    CHECK(opeu_bitmap_restore(0, 0, 1, 1, 5, img) == -1, "mode 5");

    /* row doubling (bit 15): 2 source rows -> 4 lines */
    fillpx(0);
    opeu_bitmap_restore(0, 0, 1, 2, 0x8000, img);
    /* w = 1: source rows are img[0..3] (colour 1) and img[4..7] (colour 2) */
    CHECK(px(0, 0) == 1 && px(0, 1) == 1 && px(0, 2) == 2 && px(0, 3) == 2 && px(0, 4) == 0,
          "doubled rows");

    /* transparent mode 4: zero pixels keep the background */
    fillpx(6);
    opeu_bitmap_restore(0, 0, 2, 2, 4, img);
    CHECK(px(0, 1) == 15 && px(4, 1) == 6, "mode4 opaque/transparent");
    CHECK(px(8, 1) == 6 && px(12, 1) == 9, "mode4 second column");

    /* XOR mode */
    fillpx(3);
    koei_266c(0, 0, 1, 1, 1, img);
    CHECK(px(0, 0) == (3 ^ 1), "xor");
    /* AND mode / OR mode */
    fillpx(3);
    koei_266c(0, 0, 1, 1, 2, img + 4);     /* colour 2 */
    CHECK(px(0, 0) == (3 & 2), "and");
    fillpx(4);
    koei_266c(0, 0, 1, 1, 3, img + 4);
    CHECK(px(0, 0) == (4 | 2), "or");
}

static void test_blit3(void)
{
    static const uint8_t img[3] = { 0xFF, 0x0F, 0x00 };
    fillpx(8);
    opeu_blit_far(0, 0, 1, 1, 0, img);
    CHECK(px(0, 0) == 1 && px(4, 0) == 3, "blit3 plane3 cleared");
    fillpx(0);
    koei_27c2(0, 0, 1, 1, 0x4000, img);
    CHECK(px(0, 0) == 9 && px(4, 0) == 11, "blit3 plane3 set by bit 14");
    fillpx(12);                               /* planes 2,3 */
    opeu_blit_far(0, 0, 1, 1, 1, img);        /* XOR planes 0-2 only */
    CHECK(px(0, 0) == (12 ^ 1) && px(4, 0) == (12 ^ 3), "blit3 xor keeps plane 3");
    fillpx(14);
    {
        static const uint8_t m[3] = { 0xF0, 0x00, 0x00 };
        opeu_blit_far(0, 0, 1, 1, 4, m);
        CHECK(px(0, 0) == 1 && px(4, 0) == 14, "blit3 mode4 clears plane3 under mask");
    }
}

static void test_draw1(void)
{
    static const uint8_t bits[2] = { 0xA0, 0x01 };
    fillpx(7);
    opeu_draw_near(1, 2, 2, 1, 0x0005, bits);         /* opaque, colour 5 */
    CHECK(px(8, 2) == 5 && px(9, 2) == 0 && px(10, 2) == 5 && px(11, 2) == 0, "opaque");
    CHECK(px(23, 2) == 5 && px(22, 2) == 0, "opaque byte 2");

    fillpx(7);
    opeu_draw_far(1, 2, 1, 1, 0x0409, bits);          /* op 4 = set, colour 9 */
    CHECK(px(8, 2) == 9 && px(9, 2) == 7, "transparent set");

    fillpx(6);
    opeu_draw_far(1, 2, 1, 1, 0x0103, bits);          /* XOR 3 */
    CHECK(px(8, 2) == (6 ^ 3) && px(9, 2) == 6, "xor");
    fillpx(6);
    opeu_draw_far(1, 2, 1, 1, 0x0202, bits);          /* AND: 0 bits clear colour planes */
    CHECK(px(8, 2) == 6 && px(9, 2) == (6 & ~2), "and");
    fillpx(4);
    opeu_draw_far(1, 2, 1, 1, 0x0301, bits);          /* OR */
    CHECK(px(8, 2) == 5 && px(9, 2) == 4, "or");
    /* operation 7 keeps the previous selection (OR 1) and ignores colour 8 */
    fillpx(4);
    opeu_draw_far(1, 2, 1, 1, 0x0708, bits);
    CHECK(px(8, 2) == 5 && px(9, 2) == 4, "op 7 keeps previous writer state");
    /* doubled */
    fillpx(0);
    opeu_draw_far(0, 0, 1, 1, 0x800F, bits);
    CHECK(px(0, 0) == 15 && px(0, 1) == 15 && px(1, 1) == 0, "doubled 1-bit");

    /* 28C5: fg = DL, paper = DH */
    fillpx(7);
    opeu_code_0000_28c5(0, 0, 1, 1, 0x030C, bits);
    CHECK(px(0, 0) == 12 && px(1, 0) == 3 && px(2, 0) == 12 && px(7, 0) == 3, "28c5 two colours");
}

static void test_shift_glyph(void)
{
    /* 10 pixels wide: 2 source bytes per row; x = 3 */
    static const uint8_t g[2] = { 0xFF, 0xC0 };
    int x;
    fillpx(1);
    CHECK(opeu_vga_glyph(3, 0, 10, 1, 0x000E, g) == 0, "glyph ok");
    for (x = 0; x < 3; x++) CHECK(px(x, 0) == 1, "left untouched %d", x);
    for (x = 3; x < 13; x++) CHECK(px(x, 0) == 14, "glyph pixel %d", x);
    for (x = 13; x < 24; x++) CHECK(px(x, 0) == 1, "right untouched %d", x);
    /* AND with fill FF: pixels outside the glyph keep their colour */
    fillpx(15);
    opeu_gfx_blit(3, 0, 10, 1, 0x0204, g);
    /* AND: 1 bits and the FF fill keep pixels; the zero bits of the last
       source byte (glyph pixels 10..15 -> x 13..18) clear plane 2 */
    for (x = 0; x < 24; x++)
        CHECK(px(x, 0) == ((x >= 13 && x <= 18) ? 11 : 15), "and glyph x=%d got %d", x, px(x, 0));
    CHECK(opeu_vga_glyph(635, 0, 10, 1, 0, g) == -1, "glyph clip");
}

static void test_fills(void)
{
    int x;
    fillpx(0);
    CHECK(opeu_ui_box(2, 3, 2, 2, 0x0B) == 0, "box");
    CHECK(px(15, 3) == 0 && px(16, 3) == 11 && px(31, 4) == 11 && px(32, 4) == 0, "box extent");
    CHECK(opeu_vga_fill_rect(80, 0, 1, 1, 1) == -1, "box clip");

    fillpx(0);
    opeu_ui_fill(5, 10, 21, 11, 6);
    for (x = 0; x < 30; x++)
        CHECK(px(x, 10) == ((x >= 5 && x <= 21) ? 6 : 0), "fill x=%d", x);
    CHECK(px(5, 12) == 0 && px(5, 11) == 6 && px(5, 9) == 0, "fill rows");
    fillpx(0);
    opeu_ui_fill(2, 0, 4, 0, 3);                       /* single byte */
    for (x = 0; x < 16; x++) CHECK(px(x, 0) == ((x >= 2 && x <= 4) ? 3 : 0), "fill1 x=%d", x);

    fillpx(5);
    CHECK(opeu_ui_highlight(0, 0, 1, 1, 0x0103) == 0, "highlight xor");
    CHECK(px(0, 0) == 6, "highlight xor result");
    CHECK(opeu_ui_highlight(0, 0, 1, 1, 0x0003) == -1, "highlight op 0 rejected");
    fillpx(5);
    opeu_ui_highlight(0, 0, 1, 1, 0x0205);
    CHECK(px(0, 0) == 5, "highlight AND all bits: no change");

    fillpx(9);
    opeu_vga_clear();
    CHECK(px(0, -40) == 0 && px(639, 439) == 0, "clear visible");
    CHECK(vga_plane[0][0x9600] == 0xFF, "clear stops at 38400");
}

static void test_lines(void)
{
    int x, y;
    fillpx(0);
    opeu_ui_line(0, 0, 10, 0, 0x0004);
    for (x = 0; x <= 11; x++) CHECK(px(x, 0) == (x <= 10 ? 4 : 0), "hline %d", x);
    fillpx(0);
    opeu_ui_line(3, 9, 3, 0, 0x0002);                  /* vertical, reversed */
    for (y = 0; y <= 10; y++) CHECK(px(3, y) == (y <= 9 ? 2 : 0), "vline %d", y);
    fillpx(0);
    opeu_ui_line(0, 0, 4, 4, 0x0001);
    for (x = 0; x <= 4; x++) CHECK(px(x, x) == 1, "diag %d", x);
    fillpx(0);
    opeu_ui_line(0, 4, 4, 0, 0x0001);                  /* rising: step -1 */
    for (x = 0; x <= 4; x++) CHECK(px(x, 4 - x) == 1, "antidiag %d", x);
    /* shallow: 0,0 -> 4,2: err starts 2: x0 y0, x1 y1? (2-2=0 no borrow) */
    fillpx(0);
    opeu_ui_line(0, 0, 4, 2, 0x0001);
    /* di=2, dy=2, dx=4: (0,0) di=0; (1,0) borrow -> di=2, y=1; (2,1) di=0;
       (3,1) borrow -> y=2; (4,2) */
    CHECK(px(0, 0) == 1 && px(1, 0) == 1 && px(2, 1) == 1 && px(3, 1) == 1 && px(4, 2) == 1,
          "bresenham sequence");
    CHECK(px(2, 0) == 0 && px(4, 1) == 0, "bresenham off-points");
    /* dash pattern 0xAA with mode 0: off pixels drawn in colour 0 */
    fillpx(7);
    opeu_ui_line(0, 0, 7, 0, 0xAA03);
    for (x = 0; x < 8; x++) CHECK(px(x, 0) == ((x & 1) ? 0 : 3), "dash set %d", x);
    /* mode 4 (pixel |= colour) only on pattern bits */
    fillpx(8);
    opeu_ui_line(0, 0, 7, 0, 0xF041);
    for (x = 0; x < 8; x++) CHECK(px(x, 0) == (x < 4 ? 9 : 8), "dash or %d", x);
    /* XOR line */
    fillpx(5);
    opeu_ui_line(0, 0, 2, 0, 0x0013);
    CHECK(px(0, 0) == 6 && px(3, 0) == 5, "xor line");
    /* single pixel */
    fillpx(0);
    koei_2ce4(639, 399, 0x0F);
    koei_2ce4(640, 0, 0x0F);
    CHECK(px(639, 399) == 15, "plot");
    CHECK(px(0, 1) == 0, "plot clipped");
}

static void test_blit4_shift_latch_quirk(void)
{
    /* 2 source columns, shift 4.  Column 0 all colour 1 except transparent
     * right half; column 1 transparent left half. */
    static const uint8_t img[8] = { 0xF0,0,0,0,  0x0F,0,0,0 };
    int x;
    fillpx(0);
    for (x = 0; x < 24; x++) setpx(x, 0, x < 8 ? 2 : (x < 16 ? 4 : 8));
    koei_2e49(4, 0, 2, 1, img);
    /* byte 0 (x 0..7): mask 0F -> x4..7 = colour 1, x0..3 keep 2 */
    for (x = 0; x < 4; x++) CHECK(px(x, 0) == 2, "first byte keeps %d", x);
    for (x = 4; x < 8; x++) CHECK(px(x, 0) == 1, "first byte draws %d", x);
    /* byte 1 (x 8..15): data (F0 0F) >> 4 = 0x00 | mask (F0<<8|0F)>>4 = 0x00 ->
       all bits from the latch of byte 0 (colour 2), not its own colour 4 */
    for (x = 8; x < 16; x++) CHECK(px(x, 0) == 2, "middle byte gets first byte's latch %d", x);
    /* final partial byte (x 16..23): mask = 0F << 4 = F0 with fresh latch:
       x16..19 colour 1, x20..23 keep 8 */
    for (x = 16; x < 20; x++) CHECK(px(x, 0) == 1, "tail draws %d", x);
    for (x = 20; x < 24; x++) CHECK(px(x, 0) == 8, "tail keeps %d", x);
}

static void test_screen_copy(void)
{
    int x;
    fillpx(0);
    for (x = 0; x < 24; x++) setpx(x, 0, x / 8 + 1);  /* bytes: 1, 2, 3 */
    opeu_screen_copy(0, 0, 2, 1, 1, 0);               /* overlapping, dest right */
    CHECK(px(0, 0) == 1 && px(8, 0) == 1 && px(16, 0) == 2, "backward copy = memmove");
    fillpx(0);
    for (x = 0; x < 24; x++) setpx(x, 0, x / 8 + 1);
    opeu_vga_copy(1, 0, 2, 1, 0, 0);                  /* overlapping, dest left */
    CHECK(px(0, 0) == 2 && px(8, 0) == 3 && px(16, 0) == 3, "forward copy");
    fillpx(0);
    setpx(0, 0, 5);
    opeu_screen_copy(0, 0, 1, 1, 3, 2);
    CHECK(px(24, 2) == 5, "copy to another line");
}

static void test_picture(void)
{
    /* plain: 8x2 picture at (8, 3) = 2 units per row.
       row 0: literal n=2 value p=0x5? -> byte 0x15 0x?? : n = ((1)&7)+1 = 2,
              plane2 = 5&? ... use b=0x14, q=0x21: units value 0x421:
              plane2=4, plane1=2, plane0=1 (bit patterns of 4 pixels).
       row 1: copy from 1 row up, 2 units: 1 1 00 0001 = 0xC1. */
    static const uint8_t plain[] = { 8,0, 2,0, 0x14,0x21, 0xC1 };
    static const uint8_t masked[] = {
        4,0, 1,0,
        0x34,0x12, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0,       /* run 0 = 0x1234 */
        0x40                                                 /* run 0, 1 unit */
    };
    static const uint8_t masked2[] = {
        8,0, 1,0,
        0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0,
        0x38, 0xAB, 0xCD,      /* 0111 000 -> 1 unit, value ABCD */
        0x80                   /* copy 1 unit from 1 back */
    };
    static const uint8_t lz[] = { 16,0, 1,0, 0x00,0x0F, 0x82 };   /* 1 lit + copy 3 */
    int x;
    fillpx(0);
    stream = plain;
    CHECK(opeu_picture_decode(8, 3, rd) == 0, "plain decode ok");
    /* plane0 nibble 1 = 0001: pixel 3 only; plane1 nibble 2 = 0010: pixel 2;
       plane2 nibble 4 = 0100: pixel 1 */
    for (x = 0; x < 8; x++) {
        int e = (x % 4 == 3) ? 1 : (x % 4 == 2) ? 2 : (x % 4 == 1) ? 4 : 0;
        CHECK(px(8 + x, 3) == e, "plain row0 x=%d got %d want %d", x, px(8 + x, 3), e);
        CHECK(px(8 + x, 4) == e, "plain row1 copy-up x=%d", x);
    }
    CHECK(stream == plain + sizeof plain, "plain stream consumed");

    fillpx(0);
    stream = masked;
    CHECK(opeu_picture_decode_masked(4, 0, rd) == 0, "masked decode ok");
    /* value 0x1234: plane0=4 (0100) pixel1, plane1=3 (0011) pixels 2,3,
       plane2=2 (0010) pixel 2, plane3=1 (0001) pixel 3 */
    CHECK(px(4, 0) == 0 && px(5, 0) == 1 && px(6, 0) == 6 && px(7, 0) == 10,
          "masked run table: %d %d %d %d", px(4, 0), px(5, 0), px(6, 0), px(7, 0));

    fillpx(0);
    stream = masked2;
    opeu_picture_decode_masked(0, 0, rd);
    /* ABCD: plane0=D(1101) plane1=C(1100) plane2=B(1011) plane3=A(1010) */
    {
        int e0 = 1|2|4|8, e1 = 1|2|0|0, e2 = 0|0|4|8, e3 = 1|0|4|0;
        CHECK(px(0, 0) == e0 && px(1, 0) == e1 && px(2, 0) == e2 && px(3, 0) == e3,
              "full literal %d %d %d %d", px(0, 0), px(1, 0), px(2, 0), px(3, 0));
        CHECK(px(4, 0) == e0 && px(5, 0) == e1 && px(6, 0) == e2 && px(7, 0) == e3,
              "copy 1 unit back (odd parity, rotate path)");
    }

    fillpx(0);
    stream = lz;
    opeu_picture_decode(0, 0, rd);
    for (x = 0; x < 16; x++) CHECK(px(x, 0) == 1, "lz repeat x=%d", x);

    {
        static const uint8_t bad[] = { 0x81,0x02 };
        stream = bad;
        CHECK(opeu_picture_decode(0, 0, rd) == -1, "width 641 rejected");
    }
}

static void test_palette_render(void)
{
    static const uint8_t rgb[48] = { 1,2,3,  15,0,15 };
    static uint8_t out[640 * 480];
    static uint8_t pal[768];
    int w;
    vgalib_init();
    koei_2fea();
    koei_vga_identity_palette();
    CHECK(opeu_set_palette(1, 2, rgb) == 0, "set palette");
    /* entry 1: R = b1<<2 = 8, G = b2<<2 = 12, B = b0<<2 = 4 */
    CHECK(vga_dac[3] == 8 && vga_dac[4] == 12 && vga_dac[5] == 4, "dac entry 1");
    CHECK(vga_dac[6] == 0 && vga_dac[7] == 60 && vga_dac[8] == 60, "dac entry 2");
    opeu_set_palette(15, 2, rgb);                      /* runs past 15 */
    CHECK(vga_dac[16 * 3] == 0 && vga_dac[16 * 3 + 1] == 60, "dac entry 16 written");
    opeu_set_palette(4, 20, rgb);                      /* count -> 16-4 = 12 (reads 12 entries) */

    setpx(0, 0, 1);
    setpx(639, 399, 2);
    w = vgalib_render(out, pal);
    CHECK(w == 1, "render dirty");
    CHECK(out[40 * 640] == 1 && out[439 * 640 + 639] == 2 && out[0] == 0, "render pixels");
    CHECK(pal[3] == ((8 << 2) | (8 >> 4)) && pal[4] == ((12 << 2) | (12 >> 4)), "palette scale");
    CHECK(vgalib_render(out, pal) == 0, "render clean");
    vga_attr[1] = 0x3F;
    vga_dirty = 1;
    vgalib_render(out, pal);
    CHECK(out[40 * 640] == 0x3F, "attribute palette applied");
    opeu_text_mode();
    vgalib_render(out, pal);
    CHECK(out[40 * 640] == 0, "text mode renders blank");
}

/* Run by services.test/c-suites.cpp; returns non-zero on a failure. */
int opeu_test_vgalib(void)
{
    vgalib_init();
    koei_2fea();
    test_blit4();
    test_blit3();
    test_draw1();
    test_shift_glyph();
    test_fills();
    test_lines();
    test_blit4_shift_latch_quirk();
    test_screen_copy();
    test_picture();
    test_palette_render();
    printf("%d checks, %d failures\n", checks, failures);
    return failures != 0;
}
