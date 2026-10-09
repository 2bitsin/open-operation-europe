/* Standalone checks of syslib_gfx.c against values worked out by hand from
 * the original instructions (syslib.asm addresses in the comments). */
#include "syslib-gfx.hpp"
#include "vgalib.hpp"
#include <stdio.h>
#include <string.h>

static int failures;
#define CHECK(cond)                                         \
  do {                                                      \
    if (!(cond)) {                                          \
      printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #cond); \
      failures++;                                           \
    }                                                       \
  } while (0)

static void clear(void) {
  memset(vga_plane, 0, sizeof vga_plane);
}

static void test_restore_odd(void)            /* 354F */
{
  uint8_t buf[520];
  uint16_t di = 40 * 80 + 2;
  int p;
  clear();
  memset(buf, 0, sizeof buf);
  for (p = 0; p < 4; p++) memset(&vga_plane[p][di], 0xAA, 6);
  buf[0] = 0x12;
  buf[1] = 0x34;
  buf[2] = 0x56;
  buf[3] = 0x78;
  buf[4] = 0x9A;
  opeu_vram_restore_odd(2, 0, buf);
  CHECK(vga_plane[0][di] == 0xA1);
  CHECK(vga_plane[0][di + 1] == 0x23);
  CHECK(vga_plane[0][di + 2] == 0x45);
  CHECK(vga_plane[0][di + 3] == 0x67);
  CHECK(vga_plane[0][di + 4] == 0x89);
  CHECK(vga_plane[0][di + 5] == 0xAA);
  CHECK(vga_plane[1][di] == 0xA0 && vga_plane[1][di + 5] == 0x0A);
  CHECK(vga_plane[1][di + 6] == 0x00);
}

static void test_save_odd_and_roundtrip(void) /* 3882, 37E4, 34B8 */
{
  static const uint8_t row[6] = { 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC };
  uint8_t buf[520], buf2[520];
  unsigned i;
  clear();
  memcpy(&vga_plane[0][3200], row, 6);
  opeu_vram_save_odd(0, 0, buf);
  CHECK(buf[0] == 0x23 && buf[1] == 0x45 && buf[2] == 0x67 && buf[3] == 0x89 && buf[4] == 0xAB);
  CHECK(buf[5] == 0);
  for (i = 0; i < 520; i++) buf[i] = (uint8_t)(i * 7 + 3);
  opeu_vram_restore(10, 100, buf);
  CHECK(vga_plane[2][(100 + 40) * 80 + 10 + 80 + 3] == buf[20 + 10 + 3]);
  opeu_vram_save(10, 100, buf2);
  CHECK(memcmp(buf, buf2, 520) == 0);
  opeu_vram_restore_odd(20, 50, buf);
  opeu_vram_save_odd(20, 50, buf2);
  CHECK(memcmp(buf, buf2, 520) == 0);
}

static void test_glyph_color_spill(void)      /* 3A4D: word ops cross plane blocks */
{
  uint8_t v[540], exp[540];
  uint8_t g[6] = { 0x81, 0, 0, 0, 0, 0 };
  memset(v, 0xFF, sizeof v);
  memset(exp, 0xFF, sizeof exp);
  exp[4] = 0xBF;
  exp[5] = 0x7F;
  exp[14] = 0xBF;
  exp[15] = 0x7F;
  exp[19] = 0xBF;
  exp[20] = 0x7F;
  opeu_vram_glyph_color(v, g, 33, 0, 0);
  CHECK(memcmp(v, exp, sizeof v) == 0);
  memset(v, 0, sizeof v);
  opeu_vram_glyph(v, g, 0, 1);              /* planes 0-2 get 0x81, plane 3 cleared */
  CHECK(v[20] == 0x81 && v[25] == 0x81 && v[30] == 0x81 && v[35] == 0 && v[21] == 0);
}

static void test_sprite_compose(void)         /* 3E0A */
{
  static const uint8_t mask[2] = { 0xF0, 0x0F };
  static const uint8_t img[6] = { 0x01, 0x02, 0x04, 0x10, 0x20, 0x40 };
  uint8_t buf[16];
  clear();
  vga_plane[0][3200] = 0xFF;
  vga_plane[0][3201] = 0xFF;
  vga_plane[1][3200] = 0xFF;
  vga_plane[1][3201] = 0x00;
  vga_plane[2][3200] = 0x00;
  vga_plane[2][3201] = 0xFF;
  vga_plane[3][3200] = 0x0F;
  vga_plane[3][3201] = 0xF0;
  opeu_sprite_compose(0, 0, 2, 1, mask, img, buf, 0);
  CHECK(vga_plane[0][3200] == 0xF1 && vga_plane[0][3201] == 0x1F);
  CHECK(vga_plane[1][3200] == 0xF2 && vga_plane[1][3201] == 0x20);
  CHECK(vga_plane[2][3200] == 0x04 && vga_plane[2][3201] == 0x4F);
  CHECK(vga_plane[3][3200] == 0x00 && vga_plane[3][3201] == 0x00);
  CHECK(buf[0] == 0xF1 && buf[3] == 0x20 && buf[6] == 0x00);
}

static void test_3f26(void)                   /* 3F26 with a 4-pixel shift */
{
  static const uint8_t mask[1] = { 0x00 };
  static const uint8_t img[4] = { 0x0F, 0xF0, 0x3C, 0xC3 };
  uint8_t buf[8];
  int p;
  clear();
  for (p = 0; p < 4; p++) vga_plane[p][3200] = vga_plane[p][3201] = 0xFF;
  koei_3f26(4, 0, 8, 1, mask, img, buf, 1);
  CHECK(vga_plane[0][3200] == 0xF0 && vga_plane[0][3201] == 0xFF);
  CHECK(vga_plane[1][3200] == 0xFF && vga_plane[1][3201] == 0x0F);
  CHECK(vga_plane[2][3200] == 0xF3 && vga_plane[2][3201] == 0xCF);
  CHECK(vga_plane[3][3200] == 0xFC && vga_plane[3][3201] == 0x3F);
  /* flags bit 0 clear: the original never returns; the port leaves VRAM alone */
  for (p = 0; p < 4; p++) vga_plane[p][3200] = vga_plane[p][3201] = 0xFF;
  koei_3f26(4, 0, 8, 1, mask, img, buf, 0);
  CHECK(vga_plane[0][3200] == 0xFF && vga_plane[3][3201] == 0xFF);
}

static void test_copy_and_fill(void)          /* 3BC5 overlap, 3B77 */
{
  clear();
  vga_plane[0][3200] = 1;
  vga_plane[0][3201] = 2;
  vga_plane[0][3202] = 3;
  vga_plane[0][3203] = 4;
  koei_3bc5(0, 0, 1, 0);
  CHECK(vga_plane[0][3201] == 1 && vga_plane[0][3202] == 2 && vga_plane[0][3203] == 2);
  clear();
  koei_3b77(5, 2, 3, 2, 0x1A5);
  CHECK(vga_plane[3][42 * 80 + 5] == 0xA5 && vga_plane[3][43 * 80 + 7] == 0xA5);
  CHECK(vga_plane[3][43 * 80 + 8] == 0 && vga_plane[3][44 * 80 + 5] == 0);
  CHECK(vga_plane[0][42 * 80 + 5] == 0);
}

static void test_load_draw(void)              /* 3B1E, 3A04 */
{
  uint8_t v[520], m[130], s[520];
  unsigned i;
  memset(v, 0xFF, sizeof v);
  for (i = 0; i < 130; i++) m[i] = (uint8_t)(i | 0x80);
  opeu_vram_load(v, m);
  CHECK(v[0] == 0x80 && v[5] == 0x80 && v[15 + 4] == 0x84 && v[20] == 0x85 && v[519] == (uint8_t)(129 | 0x80));
  memset(s, 0x01, sizeof s);
  opeu_vram_draw(v, s);
  CHECK(v[0] == 0x81 && v[519] == 0x81);
}

/* Run by services.test/c-suites.cpp; returns non-zero on a failure. */
int opeu_test_syslib_gfx(void) {
  test_restore_odd();
  test_save_odd_and_roundtrip();
  test_glyph_color_spill();
  test_sprite_compose();
  test_3f26();
  test_copy_and_fill();
  test_load_draw();
  if (failures) {
    printf("%d failure(s)\n", failures);
    return 1;
  }
  printf("syslib_gfx: all tests passed\n");
  return 0;
}
