/* OPEN 03ac:0004..0e7b (image 0x3ac4..0x493b, 3704 bytes): the opening
 * program, first half: timing helpers, the KOEI logo cards, the prologue,
 * the colour cycle and flicker/dissolve scenes and the panorama scene.
 * The second half of the same code segment (03ac:0e7c.., the units scene
 * onward) follows below in this same file (one translation unit).
 *
 * Timing: counter A (int 66h F001/F000) drives the uninterruptible
 * opeu_open_delay; counter B (int 66h F100) is the music timeline, counting
 * down, which opeu_open_wait (relative) and opeu_open_wait_until (absolute)
 * poll together with the input (key or mouse button -> abort, result 1).
 * Every abortable scene returns 1 when aborted, 0 when it completed.
 * Pictures come from OPENGRP.NPK by absolute file position; captures of
 * the screen go to the six 64 KB far buffers whose segments are DS:1c22..
 * 1c2c; palettes are 48-byte rows of the table at DS:0444.
 *
 * The DOS build is byte-exact with MSC 6.0a (cl /AM /Gr /Gs /O), including
 * every fixup target.  Spelling notes, each proved against the bytes:
 * - opeu_open_delay only waits when ticks != 0.
 * - The capture offsets walked by the descend, march and scroll loops are
 *   near `uint8_t *` (FAROFF) variables: with int offsets C 6.0a merges the
 *   zero initialisations through AX and keeps the first loop test, which the
 *   original lacks.  Local names follow the compiler's slot order.
 * - opeu_open_panorama_flashes declares a second, unused int: the original
 *   frame reserves 4 bytes but uses only [bp-2].
 *
 * The host build receives DS (uint8_t ds[65536]) and a service callback
 * event(ds,id,args,count); every routine outside this file is a service
 * boundary (ids below, arguments in the original order; results in AX):
 *  0 fm_wait_start(ticks)            0000:133e
 *  1 sfx_busy() -> AX                0000:134b (counter A)
 *  2 fm_busy() -> AX                 0000:1337 (counter B)
 *  3 input_poll() -> AX              062d:032e
 *  4 vga_clear()                     0000:0913
 *  5 palette_fade(mode,palette)->AX  062d:0262
 *  6 palette_step(palette) -> AX     062d:01fe
 *  7 palette_set(palette)            062d:01bc
 *  8 fade_out() -> AX                062d:0304 (abortable fade to black)
 *  9 fade_out_wait() -> AX           062d:0310 (uninterruptible)
 * 10 picture_at(x,y,pos lo,pos hi)   062d:018e
 * 11 gfx_wait()                      032a:0048
 * 12 music_start(track)              0349:00a4
 * 13 music_command4(value)           0349:0020 (MAIN name: finish_side_initialization)
 * 14 dos_seek(handle,lo,hi,origin)   0000:009b
 * 15 dos_read_far(handle,seg,off,n)  0000:0057
 * 16 bitmap_restore(x,y,w,h,mode,seg,off) 0000:0688
 * 17 blit_far(x,y,w,h,mode,seg,off)  0000:044c
 * 18 bitmap_save(x,y,w,h,seg,off)    0000:1b56
 * 19 dissolve(x,y,seg,off,pattern)   0000:1e2f
 * 20 screen_copy(x,y,w,h,to_x,to_y)  0000:1bb9
 * 21 ui_box(x,y,w,h,colour)          0000:0939
 * Near pointers are DS offsets; all arithmetic wraps at 16 bits.
 */
#include "types.hpp"
typedef uint16_t (*OpenEvent)(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
static uint16_t open_call(uint8_t* ds, OpenEvent event, uint16_t id, uint16_t count, uint16_t a0, uint16_t a1,
                          uint16_t a2, uint16_t a3, uint16_t a4, uint16_t a5, uint16_t a6) {
  uint16_t args[7];
  args[0] = a0;
  args[1] = a1;
  args[2] = a2;
  args[3] = a3;
  args[4] = a4;
  args[5] = a5;
  args[6] = a6;
  return event(ds, id, args, count);
}
static uint16_t open_word(const uint8_t* ds, uint16_t at) {
  return (uint16_t)(ds[at] | ds[(uint16_t)(at + 1)] << 8);
}
#define OPEN_STATE uint8_t *ds, OpenEvent event
#define OPEN_STATE_ uint8_t *ds, OpenEvent event,
#define OPEN_ARGS ds, event
#define OPEN_ARGS_ ds, event,
#define FAROFF uint16_t
#define UNUSED __attribute__((unused))
#define OFFSET(v) (uint16_t)v
#define EV0(id) open_call(ds, event, id, 0, 0, 0, 0, 0, 0, 0, 0)
#define EV1(id, a) open_call(ds, event, id, 1, (uint16_t)(a), 0, 0, 0, 0, 0, 0)
#define EV2(id, a, b) open_call(ds, event, id, 2, (uint16_t)(a), (uint16_t)(b), 0, 0, 0, 0, 0)
#define EV4(id, a, b, c, d) \
  open_call(ds, event, id, 4, (uint16_t)(a), (uint16_t)(b), (uint16_t)(c), (uint16_t)(d), 0, 0, 0)
#define EV5(id, a, b, c, d, e) \
  open_call(ds, event, id, 5, (uint16_t)(a), (uint16_t)(b), (uint16_t)(c), (uint16_t)(d), (uint16_t)(e), 0, 0)
#define EV6(id, a, b, c, d, e, f)                                                                        \
  open_call(ds, event, id, 6, (uint16_t)(a), (uint16_t)(b), (uint16_t)(c), (uint16_t)(d), (uint16_t)(e), \
            (uint16_t)(f), 0)
#define EV7(id, a, b, c, d, e, f, g)                                                                     \
  open_call(ds, event, id, 7, (uint16_t)(a), (uint16_t)(b), (uint16_t)(c), (uint16_t)(d), (uint16_t)(e), \
            (uint16_t)(f), (uint16_t)(g))
#define FM_WAIT_START(t) EV1(0, t)
/* The allocation failures stop the program in an empty loop; the host
 * pumps its events instead (a loop without side effects is undefined in
 * C++), so the window still closes and a headless run still stops. */
#define HANG() \
  for (;;) EV0(45)
#define SFX_BUSY() EV0(1)
#define FM_BUSY() EV0(2)
#define INPUT_POLL() EV0(3)
#define VGA_CLEAR() EV0(4)
#define PALETTE_FADE(mode, palette) EV2(5, mode, palette)
#define PALETTE_STEP(palette) EV1(6, palette)
#define PALETTE_SET(palette) EV1(7, palette)
#define FADE_OUT() EV0(8)
#define FADE_OUT_WAIT() EV0(9)
#define PICTURE_AT(x, y, position) EV4(10, x, y, (uint32_t)(position), (uint32_t)(position) >> 16)
#define GFX_WAIT() EV0(11)
#define MUSIC_START(track) EV1(12, (uint8_t)(track))
#define MUSIC_COMMAND4(value) EV1(13, (uint8_t)(value))
#define DOS_SEEK(handle, position, origin) EV4(14, handle, (uint32_t)(position), (uint32_t)(position) >> 16, origin)
#define DOS_READ_FAR(handle, s, o, n) EV4(15, handle, s, o, n)
#define BITMAP_RESTORE(x, y, w, h, m, s, o) EV7(16, x, y, w, h, m, s, o)
#define BLIT_FAR(x, y, w, h, m, s, o) EV7(17, x, y, w, h, m, s, o)
#define BITMAP_SAVE(x, y, w, h, s, o) EV6(18, x, y, w, h, s, o)
#define DISSOLVE(x, y, s, o, pattern) EV5(19, x, y, s, o, pattern)
#define SCREEN_COPY(x, y, w, h, tx, ty) EV6(20, x, y, w, h, tx, ty)
#define UI_BOX(x, y, w, h, c) EV5(21, x, y, w, h, c)
#define PALETTE(n) (uint16_t)(0x444 + 0x30 * (n))
#define LOGO_FILE open_word(ds, 0x442)
#define BUFFER_A open_word(ds, 0x1c22)
#define BUFFER_B open_word(ds, 0x1c24)
#define BUFFER_C open_word(ds, 0x1c26)
#define BUFFER_D open_word(ds, 0x1c28)
#define BUFFER_E open_word(ds, 0x1c2a)
#define BUFFER_F open_word(ds, 0x1c2c)
void opeu_open_delay(OPEN_STATE_ uint16_t ticks);
int opeu_open_wait(OPEN_STATE_ uint16_t ticks);
int opeu_open_wait_until(OPEN_STATE_ uint16_t target);
void opeu_open_clear_screen(OPEN_STATE);
int opeu_open_fade_to_palette(OPEN_STATE_ uint8_t slot, int mode);
void opeu_open_load_koei_logo(OPEN_STATE);
void opeu_open_show_koei_logo(OPEN_STATE);
void opeu_open_show_logo_caption_a(OPEN_STATE);
void opeu_open_show_logo_caption_b(OPEN_STATE);
void opeu_open_scene_koei(OPEN_STATE);
int opeu_open_prologue_pictures(OPEN_STATE);
int opeu_open_scene_prologue(OPEN_STATE);
int opeu_open_descend_animation(OPEN_STATE);
int opeu_open_scene_color_cycle(OPEN_STATE);
int opeu_open_scene_flicker(OPEN_STATE);
int opeu_open_sprite_march(OPEN_STATE);
int opeu_open_panorama_scroll(OPEN_STATE);
int opeu_open_panorama_flashes(OPEN_STATE);
int opeu_open_panorama_exit(OPEN_STATE);
int opeu_open_scene_panorama(OPEN_STATE);

void opeu_open_delay(OPEN_STATE_ uint16_t ticks) {
  if (ticks) {
    FM_WAIT_START(ticks);
    while (SFX_BUSY());
  }
}

int opeu_open_wait(OPEN_STATE_ uint16_t ticks) {
  uint16_t target;
  target = FM_BUSY();
  target -= ticks;
  do {
    if (INPUT_POLL()) return 1;
  } while (FM_BUSY() > target);
  return 0;
}

int opeu_open_wait_until(OPEN_STATE_ uint16_t target) {
  while (FM_BUSY() > target) {
    if (INPUT_POLL()) return 1;
  }
  return 0;
}

void opeu_open_clear_screen(OPEN_STATE) {
  VGA_CLEAR();
}

int opeu_open_fade_to_palette(OPEN_STATE_ uint8_t slot, int mode) {
  return PALETTE_FADE(mode, PALETTE(slot));
}

void opeu_open_load_koei_logo(OPEN_STATE) {
  DOS_SEEK(LOGO_FILE, 0L, 0);
  DOS_READ_FAR(LOGO_FILE, BUFFER_A, 0, 0xb60);
  DOS_SEEK(LOGO_FILE, 0x1860L, 0);
  DOS_READ_FAR(LOGO_FILE, BUFFER_A, 0xb60, 0x270);
}

void opeu_open_show_koei_logo(OPEN_STATE) {
  opeu_open_clear_screen(OPEN_ARGS);
  opeu_open_fade_to_palette(OPEN_ARGS_ 2, -1);
  BITMAP_RESTORE(0x1b, 0x60, 0x1a, 0x70, 2, BUFFER_A, 0);
  opeu_open_delay(OPEN_ARGS_ 60);
  BITMAP_RESTORE(0x1b, 0xe0, 0x1a, 0x18, 7, BUFFER_A, 0xb60);
  opeu_open_delay(OPEN_ARGS_ 60);
  opeu_open_wait_until(OPEN_ARGS_ 0xe98a);
  FADE_OUT_WAIT();
}

void opeu_open_show_logo_caption_a(OPEN_STATE) {
  opeu_open_clear_screen(OPEN_ARGS);
  opeu_open_fade_to_palette(OPEN_ARGS_ 2, -1);
  PICTURE_AT(0x80, 0xbc, 0L);
  opeu_open_delay(OPEN_ARGS_ 0x78);
  opeu_open_wait_until(OPEN_ARGS_ 0xe868);
  FADE_OUT_WAIT();
}

void opeu_open_show_logo_caption_b(OPEN_STATE) {
  opeu_open_clear_screen(OPEN_ARGS);
  opeu_open_fade_to_palette(OPEN_ARGS_ 2, -1);
  PICTURE_AT(0xf2, 0x76, 0x7f0L);
  PICTURE_AT(0x80, 0xba, 0x42dL);
  opeu_open_delay(OPEN_ARGS_ 0x78);
  opeu_open_wait_until(OPEN_ARGS_ 0xe50c);
  FADE_OUT_WAIT();
}

void opeu_open_scene_koei(OPEN_STATE) {
  opeu_open_load_koei_logo(OPEN_ARGS);
  opeu_open_show_koei_logo(OPEN_ARGS);
  opeu_open_show_logo_caption_a(OPEN_ARGS);
  opeu_open_show_logo_caption_b(OPEN_ARGS);
}

int opeu_open_prologue_pictures(OPEN_STATE) {
  opeu_open_clear_screen(OPEN_ARGS);
  PICTURE_AT(0, 0, 0x2955fL);
  GFX_WAIT();
  BITMAP_SAVE(0, 0, 0x50, 0x8c, BUFFER_E, 0);
  BITMAP_SAVE(0, 0x8c, 0x50, 0x8c, BUFFER_F, 0);
  opeu_open_clear_screen(OPEN_ARGS);
  PICTURE_AT(0x58, 0, 0xa81L);
  if (opeu_open_fade_to_palette(OPEN_ARGS_ 3, 2)) return 1;
  BITMAP_SAVE(10, 0, 0x3c, 0xc8, BUFFER_B, 0);
  BITMAP_SAVE(10, 0xc8, 0x3c, 0xc8, BUFFER_C, 0);
  if (opeu_open_wait(OPEN_ARGS_ 0x105)) return 1;
  if (opeu_open_wait_until(OPEN_ARGS_ 0xe25d)) return 1;
  if (FADE_OUT()) return 1;
  return 0;
}

int opeu_open_scene_prologue(OPEN_STATE) {
  if (opeu_open_wait_until(OPEN_ARGS_ 0xe4bf)) return 1;
  MUSIC_START(0);
  MUSIC_COMMAND4(1);
  if (opeu_open_prologue_pictures(OPEN_ARGS)) return 1;
  opeu_open_clear_screen(OPEN_ARGS);
  PICTURE_AT(0x90, 0x3c, 0x93f7L);
  if (opeu_open_fade_to_palette(OPEN_ARGS_ 4, 2)) return 1;
  BITMAP_SAVE(0x12, 0x3c, 0x30, 0x154, BUFFER_D, 0);
  if (opeu_open_wait(OPEN_ARGS_ 0xdc)) return 1;
  if (opeu_open_wait_until(OPEN_ARGS_ 0xdfa9)) return 1;
  if (FADE_OUT()) return 1;
  return 0;
}

int opeu_open_descend_animation(OPEN_STATE) {
  uint16_t h;
  int i;
  FAROFF off_a;
  FAROFF off_b;
  opeu_open_clear_screen(OPEN_ARGS);
  if (opeu_open_fade_to_palette(OPEN_ARGS_ 30, 2)) return 1;
  h = 0;
  off_a = OFFSET(0x2580);
  off_b = OFFSET(0x4a40);
  for (i = 0; i < 25; i++) {
    BLIT_FAR(0x22, 0, 12, h += 2, 0, BUFFER_A, (uint16_t)(off_a -= 0x60));
    if (opeu_open_wait(OPEN_ARGS_ 6)) return 1;
    if (i > 21) PALETTE_STEP(PALETTE(0));
    BLIT_FAR(0x22, 0, 12, h += 2, 0, BUFFER_A, (uint16_t)(off_a -= 0x60));
    if (opeu_open_wait(OPEN_ARGS_ 6)) return 1;
    if (i > 21) PALETTE_STEP(PALETTE(0));
    off_a -= 0xc0;
    BLIT_FAR(0x22, 0, 12, h += 2, 0, BUFFER_A, (uint16_t)(off_b -= 0x60));
    if (opeu_open_wait(OPEN_ARGS_ 6)) return 1;
    if (i > 21) PALETTE_STEP(PALETTE(0));
    BLIT_FAR(0x22, 0, 12, h += 2, 0, BUFFER_A, (uint16_t)(off_b -= 0x60));
    if (opeu_open_wait(OPEN_ARGS_ 6)) return 1;
    if (i > 21) PALETTE_STEP(PALETTE(0));
    off_b -= 0xc0;
  }
  if (FADE_OUT()) return 1;
  return 0;
}

int opeu_open_scene_color_cycle(OPEN_STATE) {
  uint8_t pal;
  int j;
  opeu_open_clear_screen(OPEN_ARGS);
  PICTURE_AT(0x58, 0, 0x121a0L);
  if (opeu_open_fade_to_palette(OPEN_ARGS_ 5, 2)) return 1;
  if (opeu_open_wait(OPEN_ARGS_ 0x103)) return 1;
  if (opeu_open_wait_until(OPEN_ARGS_ 0xdced)) return 1;
  PALETTE_SET(PALETTE(6));
  if (opeu_open_wait(OPEN_ARGS_ 5)) return 1;
  PALETTE_SET(PALETTE(7));
  if (opeu_open_wait(OPEN_ARGS_ 5)) return 1;
  PALETTE_SET(PALETTE(8));
  if (opeu_open_wait(OPEN_ARGS_ 5)) return 1;
  PALETTE_SET(PALETTE(9));
  if (opeu_open_wait(OPEN_ARGS_ 5)) return 1;
  PALETTE_SET(PALETTE(5));
  if (opeu_open_wait(OPEN_ARGS_ 0x14)) return 1;
  if (FADE_OUT()) return 1;
  opeu_open_clear_screen(OPEN_ARGS);
  PICTURE_AT(0, 0, 0x28343L);
  PICTURE_AT(0x60, 0, 0x28c1bL);
  BITMAP_SAVE(0, 0, 12, 0xc8, BUFFER_A, 0);
  BITMAP_SAVE(12, 0, 12, 0xc8, BUFFER_A, 0x2580);
  opeu_open_clear_screen(OPEN_ARGS);
  PICTURE_AT(0, 0, 0x27b7dL);
  for (pal = 11, j = 0; j < 19; j++, pal++) {
    PALETTE_SET(PALETTE(pal));
    if (opeu_open_wait(OPEN_ARGS_ 5)) return 1;
  }
  if (opeu_open_fade_to_palette(OPEN_ARGS_ 0, 1)) return 1;
  if (opeu_open_descend_animation(OPEN_ARGS)) return 1;
  return 0;
}

int opeu_open_scene_flicker(OPEN_STATE) {
  int i, off;
  opeu_open_clear_screen(OPEN_ARGS);
  GFX_WAIT();
  BLIT_FAR(0, 0x3c, 0x50, 0x8c, 0, BUFFER_E, 0);
  BLIT_FAR(0, 0xc8, 0x50, 0x8c, 0, BUFFER_F, 0);
  if (opeu_open_fade_to_palette(OPEN_ARGS_ 31, 2)) return 1;
  for (i = 0; i < 2; i++) {
    PALETTE_SET(PALETTE(32));
    if (opeu_open_wait(OPEN_ARGS_ 6)) return 1;
    PALETTE_SET(PALETTE(31));
    if (opeu_open_wait(OPEN_ARGS_ 6)) return 1;
  }
  for (i = 0; i < 10; i++) {
    PALETTE_SET(PALETTE(32));
    if (opeu_open_wait(OPEN_ARGS_ 5)) return 1;
    PALETTE_SET(PALETTE(31));
    if (opeu_open_wait(OPEN_ARGS_ 8)) return 1;
  }
  if (opeu_open_wait(OPEN_ARGS_ 0x78)) return 1;
  if (FADE_OUT()) return 1;
  opeu_open_clear_screen(OPEN_ARGS);
  PICTURE_AT(0, 0, 0x399b9L);
  BITMAP_SAVE(0, 0, 0x12, 0x78, BUFFER_A, 0);
  BITMAP_SAVE(0x12, 0, 0x12, 0x78, BUFFER_A, 0x21c0);
  BITMAP_SAVE(0x24, 0, 0x12, 0x78, BUFFER_A, 0x4380);
  BITMAP_SAVE(0, 0x78, 0x12, 0x78, BUFFER_A, 0x6540);
  BITMAP_SAVE(0x12, 0x78, 0x12, 0x78, BUFFER_A, 0x8700);
  BITMAP_SAVE(0x24, 0x78, 0x12, 0x78, BUFFER_A, 0xa8c0);
  opeu_open_clear_screen(OPEN_ARGS);
  BLIT_FAR(0x12, 0x3c, 0x30, 0x154, 0, BUFFER_D, 0);
  if (opeu_open_wait(OPEN_ARGS_ 0x3c)) return 1;
  PICTURE_AT(0xc, 0x3c, 0x37110L);
  BITMAP_SAVE(0, 0x3c, 0x26, 0x120, BUFFER_E, 0);
  opeu_open_clear_screen(OPEN_ARGS);
  BLIT_FAR(0x12, 0x3c, 0x30, 0x154, 0, BUFFER_D, 0);
  if (opeu_open_fade_to_palette(OPEN_ARGS_ 4, 2)) return 1;
  if (opeu_open_wait_until(OPEN_ARGS_ 0xd56a)) return 1;
  DISSOLVE(0, 0x3c, BUFFER_E, 0, 0x0001);
  DISSOLVE(0, 0x3c, BUFFER_E, 0, 0x0101);
  DISSOLVE(0, 0x3c, BUFFER_E, 0, 0x1111);
  DISSOLVE(0, 0x3c, BUFFER_E, 0, 0x5555);
  DISSOLVE(0, 0x3c, BUFFER_E, 0, 0x7777);
  DISSOLVE(0, 0x3c, BUFFER_E, 0, 0x7f7f);
  DISSOLVE(0, 0x3c, BUFFER_E, 0, 0xffff);
  if (opeu_open_wait(OPEN_ARGS_ 0xc8)) return 1;
  if (opeu_open_wait_until(OPEN_ARGS_ 0xd3a2)) return 1;
  if (FADE_OUT()) return 1;
  opeu_open_clear_screen(OPEN_ARGS);
  PICTURE_AT(0, 0, 0x3e98bL);
  for (off = 0, i = 78; i >= 0; i--) {
    BITMAP_SAVE(i, 0, 1, 0xb0, BUFFER_D, (uint16_t)off);
    off += 0x2c0;
  }
  return 0;
}

int opeu_open_sprite_march(OPEN_STATE) {
  uint16_t d;
  int pass, j;
  FAROFF off_a;
  FAROFF off_b;
  opeu_open_clear_screen(OPEN_ARGS);
  BLIT_FAR(10, 0, 0x3c, 0xc8, 0, BUFFER_B, 0);
  BLIT_FAR(10, 0xc8, 0x3c, 0xc8, 0, BUFFER_C, 0);
  if (opeu_open_fade_to_palette(OPEN_ARGS_ 3, 2)) return 1;
  if (opeu_open_wait_until(OPEN_ARGS_ 0xd2ed)) return 1;
  for (d = 0x40, pass = 0;; d >>= 1, d++, pass++) {
    for (off_a = 0, off_b = OFFSET(0x6540), j = 0; j < 3; j++) {
      BLIT_FAR(10, 0x118, 0x12, 0x78, 0, BUFFER_A, (uint16_t)off_a);
      BLIT_FAR(0x34, 0x118, 0x12, 0x78, 0, BUFFER_A, (uint16_t)off_b);
      off_a += 0x21c0;
      off_b += 0x21c0;
      if (opeu_open_wait(OPEN_ARGS_ d)) return 1;
    }
    if (pass > 40 && !PALETTE_STEP(PALETTE(0))) {
      if (FADE_OUT()) return 1;
      break;
    }
  }
  return 0;
}

int opeu_open_panorama_scroll(OPEN_STATE) {
  int i;
  FAROFF off;
  opeu_open_clear_screen(OPEN_ARGS);
  PICTURE_AT(0, 0, 0x40bd5L);
  BITMAP_SAVE(0, 0, 0x20, 0xb0, BUFFER_A, 0);
  if (opeu_open_wait(OPEN_ARGS_ 1)) return 1;
  PICTURE_AT(0, 0, 0x421d5L);
  BITMAP_SAVE(0, 0, 0x20, 0xb0, BUFFER_B, 0);
  if (opeu_open_wait(OPEN_ARGS_ 1)) return 1;
  PICTURE_AT(0, 0, 0x43202L);
  BITMAP_SAVE(0, 0, 0x20, 0xb0, BUFFER_C, 0);
  opeu_open_clear_screen(OPEN_ARGS);
  if (opeu_open_fade_to_palette(OPEN_ARGS_ 33, 0)) return 1;
  for (off = 0, i = 0; i < 0x4f; i++) {
    SCREEN_COPY(0x19, 0x70, 0x1d, 0xb0, 0x1a, 0x70);
    BLIT_FAR(0x19, 0x70, 1, 0xb0, 0, BUFFER_D, (uint16_t)off);
    if (opeu_open_wait(OPEN_ARGS_ 5)) return 1;
    off += 0x2c0;
  }
  BITMAP_SAVE(0x19, 0x70, 0x20, 0xb0, BUFFER_D, 0);
  if (opeu_open_wait_until(OPEN_ARGS_ 0xcc15)) return 1;
  return 0;
}

int opeu_open_panorama_flashes(OPEN_STATE) {
  int i, k UNUSED;
  for (i = 0; i < 2; i++) {
    PALETTE_SET(PALETTE(34));
    PALETTE_SET(PALETTE(35));
    if (opeu_open_wait(OPEN_ARGS_ 2)) return 1;
    BLIT_FAR(0x18, 0x70, 0x20, 0xb0, 0, BUFFER_A, 0);
    PALETTE_SET(PALETTE(33));
    if (opeu_open_wait(OPEN_ARGS_ 10)) return 1;
    BLIT_FAR(0x18, 0x70, 0x20, 0xb0, 0, BUFFER_B, 0);
    if (opeu_open_wait(OPEN_ARGS_ 10)) return 1;
    BLIT_FAR(0x18, 0x70, 0x20, 0xb0, 0, BUFFER_C, 0);
    if (opeu_open_wait(OPEN_ARGS_ 10)) return 1;
    BLIT_FAR(0x18, 0x70, 0x20, 0xb0, 0, BUFFER_D, 0);
    if (i == 0 && opeu_open_wait(OPEN_ARGS_ 100)) return 1;
  }
  if (opeu_open_wait(OPEN_ARGS_ 0x1e)) return 1;
  if (opeu_open_wait_until(OPEN_ARGS_ 0xc9f8)) return 1;
  return 0;
}

int opeu_open_panorama_exit(OPEN_STATE) {
  int i;
  UI_BOX(0x38, 0x70, 1, 0xb0, 0);
  for (i = 0; i < 15; i++) {
    SCREEN_COPY(0x17, 0x70, 0x1f, 0xb0, 0x18, 0x70);
    if (opeu_open_wait(OPEN_ARGS_ 5)) return 1;
  }
  if (opeu_open_wait_until(OPEN_ARGS_ 0xc99e)) return 1;
  if (FADE_OUT()) return 1;
  PICTURE_AT(0, 0, 0x43fd2L);
  BITMAP_SAVE(0, 0, 0x14, 0xc0, BUFFER_A, 0);
  BITMAP_SAVE(0x14, 0, 0x14, 0xc0, BUFFER_A, 0x3c00);
  BITMAP_SAVE(0x28, 0, 0x14, 0xc0, BUFFER_A, 0x7800);
  BITMAP_SAVE(0x3c, 0, 0x14, 0xc0, BUFFER_A, 0xb400);
  BITMAP_SAVE(0, 0xc0, 0x14, 0xc0, BUFFER_B, 0);
  BITMAP_SAVE(0x14, 0xc0, 0x14, 0xc0, BUFFER_B, 0x3c00);
  BITMAP_SAVE(0x28, 0xc0, 0x14, 0xc0, BUFFER_B, 0x7800);
  BITMAP_SAVE(0x3c, 0xc0, 0x14, 0xc0, BUFFER_B, 0xb400);
  return 0;
}

int opeu_open_scene_panorama(OPEN_STATE) {
  if (opeu_open_sprite_march(OPEN_ARGS)) return 1;
  if (opeu_open_panorama_scroll(OPEN_ARGS)) return 1;
  if (opeu_open_panorama_flashes(OPEN_ARGS)) return 1;
  if (opeu_open_panorama_exit(OPEN_ARGS)) return 1;
  return 0;
}
/* OPEN 03ac:0e7c..2817 (image 0x493c..0x62d7, 6556 bytes): the opening
 * program, second half. It follows the first half in this file (one
 * translation unit, one code segment C03AC_OPEN) and reuses its
 * declarations, service macros and host model.
 *
 * Units scene: two vehicles drive in on 2-byte strips, then play the eleven
 * 160x192 frames listed at DS:01ae/01c4/01da (frames 8-10 loop).
 * Bombing: a bobbing bomber (bob table DS:01f0, rotating 8x64 part) fades
 * in over 58 frames synchronised to the music timeline; three explosions
 * are composed in far buffers (copy, 1-bpp mask AND, sprite OR) and played
 * with palette flashes. Flak: three flashes over picture 0x54d6d, then the
 * bomber again (DS:0250) with four flak bursts until the palette is black.
 * Artillery: ground and gun pictures are captured in columns and panned in,
 * then the gun fires shells along the DS:028e..02d6 tables with a muzzle
 * flash flicker (DS:02e2 on/off, DS:02e4 phase). Title: the colour-cycled
 * title (short form after an abort, DS:01ac), then the wait for the music
 * to end. Init/shutdown allocate and free the far buffers (DS:1c22..1c32;
 * EMS page frame as the fallback for 1c2c), read MOPENGRP.DAT, open and
 * close OPENGRP.NPK/KOEILOGO.FNT, and wait for SCESEL.DAT before leaving.
 * opeu_open_main is the program's main(): it repeats the opening until a
 * scene is aborted (then shows the short title) or the title is skipped.
 *
 * The DOS build is byte-exact with MSC 6.0a (cl /AM /Gr /Gs /O), including
 * every fixup target and DOS relocation site, compiled as one unit with
 * the first half. Spelling notes, each proved against the bytes:
 * - C 6.0a places locals by the checksum of their names, so the short
 *   local names reproduce the original frame slots. Unused locals (UNUSED)
 *   in bomb_explosions, bomber_flak and gun_setup fill the original frames'
 *   unreferenced slots.
 * - Chained zero assignments are written in the original store order.
 * - Functions ending without a return statement return the AX of their last
 *   test, which is 0 there; the original has no "sub ax,ax" in those paths.
 *   RETURN_LAST_TEST expands to nothing in DOS and to "return 0;" on the
 *   host.
 * - bomber_flak leaves its frame loop with break and fades out after it;
 *   title and main use if/else and separate ifs so that the shared tails
 *   are placed as in the original.
 *
 * Host services continue the numbering of the first half (0-21):
 * 22 far_copy(src seg,src off,seg,off,n) 0000:1c2f
 * 23 buf_or(seg,off,w,h,src seg,src off) 0000:1c74
 * 24 buf_and_mask(seg,off,w,h,mask seg,mask off) 0000:1c94
 * 25 timeline_start(ticks)           0000:132a (int 66h F101, counter B)
 * 26 fm_noop()                       0000:1352
 * 27 install_dos_handlers()          0000:1310
 * 28 dos_alloc(paragraphs) -> AX     0000:0107
 * 29 dos_free(segment)               0000:011a
 * 30 dos_close(handle)               0000:0032
 * 31 dos_exists(name) -> AX          0000:00d9
 * 32 stub_09b7(1,0)                  0000:09b7
 * 33 int86(number,AX of the block)   0000:1f40
 * 34 ui_text(text)                   02a0:071e
 * 35 cursor_print(text)              02a0:073e
 * 36 memory_release()                032a:005c
 * 37 ems_alloc_sprite() -> AX        032a:00d0
 * 38 ems_detect() -> AX              032a:00f8
 * 39 sound_initialize()              0349:0002
 * 40 music_playing() -> AX           0349:006e
 * 41 mouse_reset()                   062d:0094
 * 42 fade_alt()                      062d:031c (fade to palette 0x70)
 * 43 identity_palette()              062d:0328
 * 44 file_or_die(name) -> AX         062d:0350
 */
typedef struct {
  uint16_t ax, bx, cx, dx, si, di, cflag;
} WordRegs;
typedef struct {
  uint8_t al, ah, bl, bh, cl, ch, dl, dh;
} ByteRegs;
typedef union {
  WordRegs x;
  ByteRegs h;
} Regs;
static uint16_t open_set(uint8_t* ds, uint16_t at, uint16_t value) {
  ds[at] = (uint8_t)value;
  ds[(uint16_t)(at + 1)] = (uint8_t)(value >> 8);
  return value;
}
#define OPEN_INT(at) ((int16_t)open_word(ds, (uint16_t)(at)))
#define OPEN_TABLE(base, i) open_word(ds, (uint16_t)((base) + 2 * (i)))
#define FAR_COPY(ss, so, s, o, n) EV5(22, ss, so, s, o, n)
#define BUF_OR(s, o, w, h, ss, so) EV6(23, s, o, w, h, ss, so)
#define BUF_AND_MASK(s, o, w, h, ms, mo) EV6(24, s, o, w, h, ms, mo)
#define TIMELINE_START(t) EV1(25, t)
#define FM_NOOP() EV0(26)
#define INSTALL_HANDLERS() EV0(27)
#define DOS_ALLOC(n) EV1(28, n)
#define DOS_FREE(s) EV1(29, s)
#define DOS_CLOSE(h) EV1(30, h)
#define DOS_EXISTS(name) EV1(31, name)
#define STUB_09B7(a, b) EV2(32, a, b)
#define INT86(n, in, out) EV2(33, n, (in)->h.al | (in)->h.ah << 8)
#define UI_TEXT(text) EV1(34, text)
#define CURSOR_PRINT(text) EV1(35, text)
#define MEMORY_RELEASE() EV0(36)
#define EMS_ALLOC_SPRITE() EV0(37)
#define EMS_DETECT() EV0(38)
#define SOUND_INITIALIZE() EV0(39)
#define MUSIC_PLAYING() EV0(40)
#define MOUSE_RESET() EV0(41)
#define FADE_ALT() EV0(42)
#define IDENTITY_PALETTE() EV0(43)
#define FILE_OR_DIE(name) EV1(44, name)
#define EMS_USED open_word(ds, 0x180)
#define ABORTED open_word(ds, 0x1ac)
#define SET_ABORTED(v) open_set(ds, 0x1ac, v)
#define FRAME_Y(i) OPEN_TABLE(0x1ae, i)
#define FRAME_H(i) OPEN_TABLE(0x1c4, i)
#define FRAME_OFF(i) OPEN_TABLE(0x1da, i)
#define BOB_A(i) OPEN_TABLE(0x1f0, i)
#define BLAST_STEPS(i) OPEN_INT(0x218 + 2 * (i))
#define BLAST_SLOT(i) ds[(uint16_t)(0x224 + (i))]
#define BLAST_X(i) OPEN_TABLE(0x228, i)
#define BLAST_Y(i) OPEN_TABLE(0x22e, i)
#define BLAST_MASK(i) OPEN_TABLE(0x234, i)
#define BLAST_SPRITE(i) OPEN_TABLE(0x240, i)
#define FLASH_SLOT(i) ds[(uint16_t)(0x24c + (i))]
#define BOB_B(i) OPEN_TABLE(0x250, i)
#define FLAK_X(i) OPEN_INT(0x278 + 2 * (i))
#define FLAK_Y(i) OPEN_TABLE(0x280, i)
#define GUN_FLIPS(i) OPEN_INT(0x288 + 2 * (i))
#define SHELL_X(i) OPEN_TABLE(0x28e, i)
#define SHELL_Y(i) OPEN_TABLE(0x29a, i)
#define SHELL_W(i) OPEN_TABLE(0x2a6, i)
#define SHELL_WS(i) OPEN_TABLE(0x2b2, i)
#define SHELL_H(i) OPEN_TABLE(0x2be, i)
#define SHELL_SPRITE(i) OPEN_TABLE(0x2ca, i)
#define SHELL_SAVE(i) OPEN_TABLE(0x2d6, i)
#define FLASH_ON open_word(ds, 0x2e2)
#define FLASH_PHASE open_word(ds, 0x2e4)
#define FLIP_FLASH_ON() (ds[0x2e2] ^= 1)
#define FLIP_FLASH_PHASE() (ds[0x2e4] ^= 1)
#define ALLOC_ERR1 0x2e6
#define ALLOC_ERR2 0x2f1
#define ALLOC_ERR3 0x2fc
#define ALLOC_ERR4 0x307
#define ALLOC_ERR5 0x312
#define ALLOC_ERR6 0x31d
#define ALLOC_ERR7 0x328
#define ALLOC_ERR9 0x333
#define TEXT_CONSOLE 0x33e
#define NAME_SCESEL 0x34b
#define NAME_FNT 0x418
#define NAME_NPK 0x426
#define NAME_MOPENGRP 0x432
#define NPK_FILE open_word(ds, 0x440)
#define SET_NPK_FILE(v) open_set(ds, 0x440, v)
#define SET_LOGO_FILE(v) open_set(ds, 0x442, v)
#define BUFFER_G open_word(ds, 0x1c30)
#define BUFFER_H open_word(ds, 0x1c32)
#define SET_BUFFER_A(v) open_set(ds, 0x1c22, v)
#define SET_BUFFER_B(v) open_set(ds, 0x1c24, v)
#define SET_BUFFER_C(v) open_set(ds, 0x1c26, v)
#define SET_BUFFER_D(v) open_set(ds, 0x1c28, v)
#define SET_BUFFER_E(v) open_set(ds, 0x1c2a, v)
#define SET_BUFFER_F(v) open_set(ds, 0x1c2c, v)
#define SET_BUFFER_G(v) open_set(ds, 0x1c30, v)
#define SET_BUFFER_H(v) open_set(ds, 0x1c32, v)
#define RETURN_LAST_TEST return 0;
int opeu_open_units_drive_in(OPEN_STATE);
int opeu_open_units_animate(OPEN_STATE);
int opeu_open_scene_units(OPEN_STATE);
int opeu_open_bomber_approach(OPEN_STATE);
int opeu_open_bomb_explosions(OPEN_STATE);
int opeu_open_scene_bombing(OPEN_STATE);
int opeu_open_flash_preload(OPEN_STATE);
int opeu_open_bomber_flak(OPEN_STATE);
int opeu_open_scene_flak(OPEN_STATE);
int opeu_open_gun_setup(OPEN_STATE);
int opeu_open_gun_pan(OPEN_STATE);
void opeu_open_flash_toggle(OPEN_STATE);
int opeu_open_shell_run(OPEN_STATE_ int first);
int opeu_open_shell_run_pair(OPEN_STATE);
int opeu_open_gun_fire(OPEN_STATE);
int opeu_open_scene_artillery(OPEN_STATE);
int opeu_open_title(OPEN_STATE);
void opeu_open_init(OPEN_STATE);
void opeu_open_free_buffers(OPEN_STATE);
void opeu_open_shutdown(OPEN_STATE);
void opeu_open_files_open(OPEN_STATE);
void opeu_open_files_close(OPEN_STATE);
void opeu_open_main(OPEN_STATE);

/* x: strip column, off_a/off_b: strip offsets in buffer C. */
int opeu_open_units_drive_in(OPEN_STATE) {
  int x;
  uint16_t off_a, off_b;
  opeu_open_clear_screen(OPEN_ARGS);
  BLIT_FAR(2, 0, 0x14, 0x60, 0, BUFFER_A, 0);
  off_a = 0;
  for (x = 0x15; x > 0; x -= 2) {
    BITMAP_SAVE(x, 0, 2, 0x60, BUFFER_C, off_a);
    off_a += 0x300;
  }
  for (x = 0x14; x >= 0; x -= 2) {
    BITMAP_SAVE(x, 0, 2, 0x60, BUFFER_C, off_a);
    off_a += 0x300;
  }
  opeu_open_clear_screen(OPEN_ARGS);
  PALETTE_SET(PALETTE(36));
  x = off_b = off_a = 0;
  for (; x < 31; x++) {
    SCREEN_COPY(0, 0xc4, 0x3c, 0x60, 2, 0xc4);
    if (x < 11) {
      BLIT_FAR(0, 0xc4, 2, 0x60, 0, BUFFER_C, off_a);
      off_a += 0x300;
    }
    if (x > 12 && x < 24) {
      BLIT_FAR(0, 0xc4, 2, 0x60, 0, BUFFER_C, off_b);
      off_b += 0x300;
    }
    if (opeu_open_wait(OPEN_ARGS_ 6)) return 1;
  }
  if (opeu_open_wait(OPEN_ARGS_ 10)) return 1;
  RETURN_LAST_TEST
}

/* i: frame count, j/k: frame of the right/left vehicle. */
int opeu_open_units_animate(OPEN_STATE) {
  int i, j, k;
  i = k = j = 0;
  for (; i < 40; i++) {
    if (i == 0) {
      PALETTE_SET(PALETTE(34));
      PALETTE_SET(PALETTE(35));
      if (opeu_open_wait(OPEN_ARGS_ 2)) return 1;
      PALETTE_SET(PALETTE(36));
    }
    UI_BOX(0x2a, 0x64, 0x14, 0x80, 0);
    BLIT_FAR(0x2a, FRAME_Y(j), 0x14, FRAME_H(j), 0, j < 5 ? BUFFER_A : BUFFER_B, FRAME_OFF(j));
    j++;
    if (j == 11) j = 8;
    UI_BOX(0x10, 0x64, 0x14, 0x80, 0);
    BLIT_FAR(0x10, FRAME_Y(k), 0x14, FRAME_H(k), 0, k < 5 ? BUFFER_A : BUFFER_B, FRAME_OFF(k));
    if (i > 2) {
      k++;
      if (k == 11) k = 8;
    }
    if (opeu_open_wait(OPEN_ARGS_ 10)) return 1;
    if (i > 30) PALETTE_STEP(PALETTE(0));
  }
  if (FADE_OUT()) return 1;
  if (opeu_open_wait_until(OPEN_ARGS_ 0xc5e2)) return 1;
  RETURN_LAST_TEST
}

int opeu_open_scene_units(OPEN_STATE) {
  if (opeu_open_units_drive_in(OPEN_ARGS)) return 1;
  if (opeu_open_units_animate(OPEN_ARGS)) return 1;
  RETURN_LAST_TEST
}

/* a: composite toggle, b: frame deadline, c: bob index, d: frame,
 * e: y, f: faded in, g: rotating part offset, h: next y. */
int opeu_open_bomber_approach(OPEN_STATE) {
  uint16_t a;
  uint16_t b;
  int c, d;
  uint16_t e;
  int f;
  uint16_t g, h;
  opeu_open_clear_screen(OPEN_ARGS);
  PICTURE_AT(0, 0, 0x46848L);
  BITMAP_SAVE(0, 0, 0x50, 0xa0, BUFFER_C, 0);
  opeu_open_clear_screen(OPEN_ARGS);
  PICTURE_AT(0, 0, 0x487fbL);
  BITMAP_SAVE(0, 0, 8, 0x40, BUFFER_A, 0);
  BITMAP_SAVE(8, 0, 8, 0x40, BUFFER_A, 0x800);
  BITMAP_SAVE(0x10, 0, 8, 0x40, BUFFER_A, 0x1000);
  BITMAP_SAVE(0x18, 0, 8, 0x40, BUFFER_A, 0x1800);
  BITMAP_SAVE(0x20, 0, 8, 0x40, BUFFER_A, 0x2000);
  PICTURE_AT(0, 0, 0x48ce9L);
  BITMAP_SAVE(0, 0, 0x32, 0x140, BUFFER_B, 0);
  PICTURE_AT(0, 0, 0x4d847L);
  BITMAP_SAVE(0, 0, 0x1a, 0xb0, BUFFER_E, 0);
  BITMAP_SAVE(0x1a, 0, 0x1a, 0xb0, BUFFER_E, 0x4780);
  BITMAP_SAVE(0x34, 0, 0x1a, 0xb0, BUFFER_E, 0x8f00);
  GFX_WAIT();
  BITMAP_SAVE(0, 0xb0, 0x1a, 0xb0, BUFFER_F, 0);
  BITMAP_SAVE(0x1a, 0xb0, 0x1a, 0xb0, BUFFER_F, 0x4780);
  BITMAP_SAVE(0x34, 0xb0, 0x1a, 0xb0, BUFFER_F, 0x8f00);
  opeu_open_clear_screen(OPEN_ARGS);
  PICTURE_AT(0, 0, 0x47428L);
  BITMAP_SAVE(0, 0, 0x10, 0x90, BUFFER_A, 0x2800);
  BITMAP_SAVE(0x10, 0, 0x10, 0x90, BUFFER_A, 0x7000);
  BITMAP_SAVE(0x20, 0, 0x10, 0x90, BUFFER_A, 0x4c00);
  BITMAP_SAVE(0x30, 0, 0x10, 0x90, BUFFER_A, 0x9400);
  opeu_open_clear_screen(OPEN_ARGS);
  BLIT_FAR(0, 0x50, 0x50, 0xa0, 0, BUFFER_C, 0);
  BLIT_FAR(0x13, 0x50, 0x10, 0x90, 0, BUFFER_A, 0x2800);
  BLIT_FAR(0x32, 0x50, 0x10, 0x90, 0, BUFFER_A, 0x4c00);
  BLIT_FAR(0, 0xf0, 0x50, 0xa0, 0, BUFFER_C, 0);
  BLIT_FAR(0x13, 0xf0, 0x10, 0x90, 0, BUFFER_A, 0x7000);
  BLIT_FAR(0x32, 0xf0, 0x10, 0x90, 0, BUFFER_A, 0x9400);
  BITMAP_SAVE(0, 0x50, 0x50, 0xa0, BUFFER_C, 0);
  BITMAP_SAVE(0, 0xf0, 0x50, 0xa0, BUFFER_D, 0);
  opeu_open_clear_screen(OPEN_ARGS);
  BLIT_FAR(0, 0x50, 0x50, 0xa0, 0, BUFFER_D, 0);
  h = e = BOB_A(0);
  d = f = c = a = g = 0;
  for (; d < 0x3a; d++) {
    b = FM_BUSY();
    b -= 12;
    e = BOB_A(c);
    BLIT_FAR(0, e, 0x50, 0xa0, 0, a ? BUFFER_D : BUFFER_C, 0);
    BLIT_FAR(0x23, e + 0x60, 8, 0x40, 0, BUFFER_A, g);
    a ^= 1;
    c++;
    if (c > 19) c = 0;
    h = BOB_A(c);
    if (h < e) UI_BOX(0, h + 0xa0, 0x50, e - h, 0);
    g += 0x800;
    if (g >= 0x2800) g = 0;
    if (!f) {
      if (!PALETTE_STEP(PALETTE(0x25))) f = 1;
    }
    while (FM_BUSY() > b) {
      if (INPUT_POLL()) return 1;
    }
  }
  if (FADE_OUT()) return 1;
  RETURN_LAST_TEST
}

/* k: explosion, j: frame, n: palette step, pos: frame offset. */
int opeu_open_bomb_explosions(OPEN_STATE) {
  int k, j, n, p UNUSED, a UNUSED;
  uint16_t pos;
  opeu_open_clear_screen(OPEN_ARGS);
  PALETTE_SET(PALETTE(0x26));
  BLIT_FAR(0xf, 0x28, 0x32, 0x140, 0, BUFFER_B, 0);
  if (opeu_open_wait_until(OPEN_ARGS_ 0xbd4b)) return 1;
  if (opeu_open_wait(OPEN_ARGS_ 50)) return 1;
  for (k = 0; k < 3; k++) {
    BITMAP_SAVE(BLAST_X(k), BLAST_Y(k), 0x1a, 0xb0, BUFFER_A, 0xb800);
    for (pos = 0, j = 0; j < 3; j++, pos += 0x4780) {
      FAR_COPY(BUFFER_A, 0xb800, BUFFER_B, pos, 0x4780);
      BUF_AND_MASK(BUFFER_B, pos, 0x1a, 0xb0, BUFFER_H, BLAST_MASK(j));
      BUF_OR(BUFFER_B, pos, 0x1a, 0xb0, BUFFER_E, BLAST_SPRITE(j));
    }
    for (pos = 0, j = 3; j < 6; j++, pos += 0x4780) {
      FAR_COPY(BUFFER_A, 0xb800, BUFFER_C, pos, 0x4780);
      BUF_AND_MASK(BUFFER_C, pos, 0x1a, 0xb0, BUFFER_H, BLAST_MASK(j));
      GFX_WAIT();
      BUF_OR(BUFFER_C, pos, 0x1a, 0xb0, BUFFER_F, BLAST_SPRITE(j));
    }
    for (pos = 0, j = 0; j < 6; j++) {
      BLIT_FAR(BLAST_X(k), BLAST_Y(k), 0x1a, 0xb0, 4, j < 3 ? BUFFER_B : BUFFER_C, pos);
      if (pos == 0x8f00)
        pos = 0;
      else
        pos += 0x4780;
      for (n = 0; n < BLAST_STEPS(j); n++) PALETTE_STEP(PALETTE(BLAST_SLOT(k)));
      if (opeu_open_wait(OPEN_ARGS_ 2)) return 1;
    }
    BLIT_FAR(BLAST_X(k), BLAST_Y(k), 0x1a, 0xb0, 0, BUFFER_A, 0xb800);
    if (opeu_open_fade_to_palette(OPEN_ARGS_ 0x26, 1)) return 1;
    BLIT_FAR(BLAST_X(k), BLAST_Y(k), 0x1a, 0xb0, 0, BUFFER_A, 0xb800);
    if (opeu_open_wait(OPEN_ARGS_ 10)) return 1;
  }
  if (opeu_open_wait(OPEN_ARGS_ 90)) return 1;
  if (opeu_open_wait_until(OPEN_ARGS_ 0xbada)) return 1;
  if (FADE_OUT()) return 1;
  RETURN_LAST_TEST
}

int opeu_open_scene_bombing(OPEN_STATE) {
  if (opeu_open_bomber_approach(OPEN_ARGS)) return 1;
  if (opeu_open_bomb_explosions(OPEN_ARGS)) return 1;
  RETURN_LAST_TEST
}

int opeu_open_flash_preload(OPEN_STATE) {
  int i;
  opeu_open_clear_screen(OPEN_ARGS);
  PICTURE_AT(0x4c, 0xc, 0x54d6dL);
  if (opeu_open_fade_to_palette(OPEN_ARGS_ 0x2a, 2)) return 1;
  if (opeu_open_wait(OPEN_ARGS_ 100)) return 1;
  if (opeu_open_wait_until(OPEN_ARGS_ 0xb8d3)) return 1;
  for (i = 0; i < 3; i++) {
    PALETTE_SET(PALETTE(FLASH_SLOT(i)));
    while (PALETTE_STEP(PALETTE(0x2a)));
    if (opeu_open_wait(OPEN_ARGS_ 60)) return 1;
  }
  if (opeu_open_wait(OPEN_ARGS_ 30)) return 1;
  if (opeu_open_wait_until(OPEN_ARGS_ 0xb6f8)) return 1;
  if (FADE_OUT()) return 1;
  opeu_open_clear_screen(OPEN_ARGS);
  PICTURE_AT(0, 0xa, 0x5b770L);
  BITMAP_SAVE(0, 0, 6, 0x3c, BUFFER_B, 0);
  BITMAP_SAVE(6, 0, 6, 0x3c, BUFFER_B, 0x5a0);
  BITMAP_SAVE(0xc, 0, 6, 0x3c, BUFFER_B, 0xb40);
  BITMAP_SAVE(0x12, 0, 6, 0x3c, BUFFER_B, 0x10e0);
  opeu_open_clear_screen(OPEN_ARGS);
  PICTURE_AT(0, 0, 0x5b9e3L);
  BITMAP_SAVE(0, 0, 0x20, 0xe0, BUFFER_E, 0);
  PICTURE_AT(0, 0, 0x5d486L);
  BITMAP_SAVE(0, 0, 0x20, 0xe0, BUFFER_E, 0x7000);
  if (opeu_open_wait(OPEN_ARGS_ 15)) return 1;
  if (opeu_open_wait_until(OPEN_ARGS_ 0xb6f8)) return 1;
  if (FADE_OUT()) return 1;
  RETURN_LAST_TEST
}

/* a: composite toggle, b: frame % 10, d: deadline, e: bob index,
 * f: frame, g: y, h: rotating part offset, i: next y, j: burst offset. */
int opeu_open_bomber_flak(OPEN_STATE) {
  uint16_t a;
  int b, c UNUSED;
  uint16_t d;
  int e, f;
  uint16_t g, h, i, j;
  opeu_open_clear_screen(OPEN_ARGS);
  BLIT_FAR(0, 0x50, 0x50, 0xa0, 0, BUFFER_D, 0);
  BLIT_FAR(0x13, 0x50, 0x10, 0x90, 0, BUFFER_A, 0x2800);
  BLIT_FAR(0x32, 0x50, 0x10, 0x90, 0, BUFFER_A, 0x4c00);
  BITMAP_SAVE(0, 0x50, 0x50, 0xa0, BUFFER_C, 0);
  opeu_open_clear_screen(OPEN_ARGS);
  PALETTE_SET(PALETTE(0x2e));
  BLIT_FAR(0, 0x50, 0x50, 0xa0, 0, BUFFER_D, 0);
  i = g = BOB_B(0);
  f = e = j = h = a = 0;
  for (;; f++) {
    d = FM_BUSY();
    d -= 12;
    g = BOB_B(e);
    BLIT_FAR(0, g, 0x50, 0xa0, 0, a ? BUFFER_D : BUFFER_C, 0);
    BLIT_FAR(0x23, g + 0x60, 8, 0x40, 0, BUFFER_A, h);
    a ^= 1;
    e++;
    if (e > 19) e = 0;
    i = BOB_B(e);
    if (i < g) UI_BOX(0, i + 0xa0, 0x50, g - i, 0);
    b = f % 10;
    if (b > 4 && b < 9) {
      BLIT_FAR(FLAK_X(f / 10 % 4) - b + 5, FLAK_Y(f / 10 % 4), 6, 0x3c, 0, BUFFER_B, j);
      j += 0x5a0;
      if (j >= 0x1680) j = 0;
    }
    h += 0x800;
    if (h >= 0x2800) h = 0;
    if (f >= 30) {
      if (!PALETTE_STEP(PALETTE(0))) break;
    }
    while (FM_BUSY() > d) {
      if (INPUT_POLL()) return 1;
    }
  }
  if (FADE_OUT()) return 1;
  return 0;
}

int opeu_open_scene_flak(OPEN_STATE) {
  if (opeu_open_flash_preload(OPEN_ARGS)) return 1;
  if (opeu_open_bomber_flak(OPEN_ARGS)) return 1;
  RETURN_LAST_TEST
}

/* p: column / flip count, toggle: figure frame, k: salvo, o: column offset. */
int opeu_open_gun_setup(OPEN_STATE) {
  int p, toggle, k, old UNUSED;
  uint16_t o;
  opeu_open_clear_screen(OPEN_ARGS);
  PICTURE_AT(0, 0, 0x5eddeL);
  BITMAP_SAVE(0, 0, 6, 0x30, BUFFER_A, 0);
  BITMAP_SAVE(6, 0, 6, 0x30, BUFFER_A, 0x480);
  BITMAP_SAVE(0xc, 0, 6, 0x30, BUFFER_A, 0x900);
  BITMAP_SAVE(0x12, 0, 6, 0x30, BUFFER_A, 0xd80);
  opeu_open_clear_screen(OPEN_ARGS);
  BLIT_FAR(0, 0x108, 6, 0x30, 0, BUFFER_A, 0x900);
  BLIT_FAR(6, 0x108, 6, 0x30, 0, BUFFER_A, 0x900);
  BLIT_FAR(0xc, 0x108, 6, 0x30, 0, BUFFER_A, 0xd80);
  BLIT_FAR(0x12, 0x108, 6, 0x30, 0, BUFFER_A, 0x480);
  BLIT_FAR(0x18, 0x108, 6, 0x30, 0, BUFFER_A, 0x900);
  BLIT_FAR(0x1e, 0x108, 6, 0x30, 0, BUFFER_A, 0x480);
  BLIT_FAR(0x24, 0x108, 6, 0x30, 0, BUFFER_A, 0xd80);
  BLIT_FAR(0x28, 0x108, 6, 0x30, 0, BUFFER_A, 0xd80);
  BLIT_FAR(0x30, 0x108, 6, 0x30, 0, BUFFER_A, 0x900);
  BLIT_FAR(0x36, 0x108, 6, 0x30, 0, BUFFER_A, 0);
  o = 0;
  for (p = 0x35; p >= 0; p--) {
    BITMAP_SAVE(p, 0x108, 1, 0x30, BUFFER_B, o);
    o += 0xc0;
  }
  opeu_open_clear_screen(OPEN_ARGS);
  PICTURE_AT(0, 0x88, 0x5f135L);
  if (opeu_open_wait(OPEN_ARGS_ 1)) return 1;
  BLIT_FAR(0x28, 0x108, 6, 0x30, 0, BUFFER_A, 0x900);
  o = 0;
  for (p = 0x27; p >= 0; p--) {
    BITMAP_SAVE(p, 0x88, 1, 0xe0, BUFFER_C, o);
    o += 0x380;
  }
  opeu_open_clear_screen(OPEN_ARGS);
  if (opeu_open_fade_to_palette(OPEN_ARGS_ 0x2f, 2)) return 1;
  BLIT_FAR(0x18, 0x58, 0x20, 0xe0, 0, BUFFER_E, 0);
  BLIT_FAR(0x12, 0x108, 6, 0x30, 0, BUFFER_A, 0);
  BLIT_FAR(0xc, 0x108, 6, 0x30, 0, BUFFER_A, 0x900);
  BLIT_FAR(6, 0x108, 6, 0x30, 0, BUFFER_A, 0x480);
  BLIT_FAR(0, 0x108, 6, 0x30, 0, BUFFER_A, 0);
  BITMAP_SAVE(1, 0x58, 0x36, 0xe0, BUFFER_D, 0);
  if (opeu_open_wait(OPEN_ARGS_ 100)) return 1;
  if (opeu_open_wait_until(OPEN_ARGS_ 0xb121)) return 1;
  for (k = toggle = 0; k < 3; k++) {
    for (p = 0; p < GUN_FLIPS(k); p++) {
      BLIT_FAR(0x18, 0x58, 0x20, 0xe0, 0, BUFFER_E, toggle ? 0 : 0x7000);
      if (opeu_open_wait(OPEN_ARGS_ toggle ? 2 : 4)) return 1;
      toggle ^= 1;
    }
    if (opeu_open_wait(OPEN_ARGS_ 100)) return 1;
  }
  return 0;
}

int opeu_open_gun_pan(OPEN_STATE) {
  int i;
  uint16_t off;
  for (off = 0, i = 0; i < 0x36; i++) {
    SCREEN_COPY(0, 0x58, 0x36, 0xe0, 1, 0x58);
    BLIT_FAR(0, 0x108, 1, 0x30, 0, BUFFER_B, off);
    off += 0xc0;
    if (INPUT_POLL()) return 1;
  }
  for (off = 0, i = 0; i < 0x28; i++) {
    SCREEN_COPY(0, 0x58, 0x36, 0x110, 1, 0x58);
    BLIT_FAR(0, 0x88, 1, 0xe0, 0, BUFFER_C, off);
    off += 0x380;
    if (INPUT_POLL()) return 1;
  }
  if (opeu_open_wait(OPEN_ARGS_ 15)) return 1;
  if (opeu_open_wait_until(OPEN_ARGS_ 0xac2d)) return 1;
  RETURN_LAST_TEST
}

void opeu_open_flash_toggle(OPEN_STATE) {
  if (FLASH_ON) {
    if (FLASH_PHASE)
      PALETTE_SET(PALETTE(0x2f));
    else
      PALETTE_SET(PALETTE(0x30));
    FLIP_FLASH_PHASE();
  }
}

int opeu_open_shell_run(OPEN_STATE_ int first) {
  int i;
  for (i = first; i < first + 3; i++) {
    BITMAP_RESTORE(SHELL_X(i), SHELL_Y(i), SHELL_W(i), SHELL_H(i), 0x407, BUFFER_H, SHELL_SPRITE(i));
    if (opeu_open_wait(OPEN_ARGS_ 1)) return 1;
    BLIT_FAR(SHELL_X(i), SHELL_Y(i), SHELL_WS(i), SHELL_H(i), 0, BUFFER_A, SHELL_SAVE(i));
    opeu_open_flash_toggle(OPEN_ARGS);
  }
  return 0;
}

int opeu_open_shell_run_pair(OPEN_STATE) {
  int i, j;
  j = 3;
  for (i = 0; i < 3; i++, j++) {
    BITMAP_RESTORE(SHELL_X(i), SHELL_Y(i), SHELL_W(i), SHELL_H(i), 0x407, BUFFER_H, SHELL_SPRITE(i));
    BITMAP_RESTORE(SHELL_X(j), SHELL_Y(j), SHELL_W(j), SHELL_H(j), 0x407, BUFFER_H, SHELL_SPRITE(j));
    if (opeu_open_wait(OPEN_ARGS_ 1)) return 1;
    BLIT_FAR(SHELL_X(i), SHELL_Y(i), SHELL_WS(i), SHELL_H(i), 0, BUFFER_A, SHELL_SAVE(i));
    BLIT_FAR(SHELL_X(j), SHELL_Y(j), SHELL_WS(j), SHELL_H(j), 0, BUFFER_A, SHELL_SAVE(j));
    opeu_open_flash_toggle(OPEN_ARGS);
  }
  return 0;
}

/* runs: pair salvos left, frame: frame, shot: 1 first, 2 pairs, 3 last. */
int opeu_open_gun_fire(OPEN_STATE) {
  int runs, frame, shot;
  for (frame = 0; frame < 6; frame++)
    BITMAP_SAVE(SHELL_X(frame), SHELL_Y(frame), SHELL_WS(frame), SHELL_H(frame), BUFFER_A, SHELL_SAVE(frame));
  frame = runs = shot = 0;
  for (; frame < 50; frame++) {
    if (frame == 7) {
      runs = 3;
      shot = 1;
    }
    if (frame == 15) {
      runs = 10;
      shot = 1;
    }
    if (frame == 35) {
      runs = 5;
      shot = 1;
    }
    if (frame == 3 || frame == 12 || frame == 20 || frame == 23 || frame == 40 || frame == 49) FLIP_FLASH_ON();
    if (shot == 1) {
      opeu_open_shell_run(OPEN_ARGS_ 0);
      shot = 2;
    } else if (shot == 2) {
      opeu_open_shell_run_pair(OPEN_ARGS);
      if (--runs == 0) shot = 3;
    } else if (shot == 3) {
      opeu_open_shell_run(OPEN_ARGS_ 3);
      shot = 0;
    }
    if (!FLASH_ON)
      PALETTE_SET(PALETTE(0x2f));
    else
      opeu_open_flash_toggle(OPEN_ARGS);
    if (opeu_open_wait(OPEN_ARGS_ 1)) return 1;
  }
  if (opeu_open_wait(OPEN_ARGS_ 0x50)) return 1;
  if (opeu_open_wait_until(OPEN_ARGS_ 0xa8e2)) return 1;
  if (FADE_OUT()) return 1;
  RETURN_LAST_TEST
}

int opeu_open_scene_artillery(OPEN_STATE) {
  if (opeu_open_gun_setup(OPEN_ARGS)) return 1;
  if (opeu_open_gun_pan(OPEN_ARGS)) return 1;
  if (opeu_open_gun_fire(OPEN_ARGS)) return 1;
  RETURN_LAST_TEST
}

int opeu_open_title(OPEN_STATE) {
  int i, start;
  opeu_open_clear_screen(OPEN_ARGS);
  start = ABORTED ? 0x65 : 0x31;
  FADE_OUT_WAIT();
  opeu_open_fade_to_palette(OPEN_ARGS_ 0, 2);
  FADE_OUT();
  if (ABORTED) {
    opeu_open_fade_to_palette(OPEN_ARGS_ 0x6f, 2);
    opeu_open_delay(OPEN_ARGS_ 100);
  }
  PICTURE_AT(0x37, 0x3c, 0x5ff59L);
  for (i = start; i <= 0x6f; i++) {
    opeu_open_fade_to_palette(OPEN_ARGS_ i, 2);
    if (ABORTED)
      opeu_open_delay(OPEN_ARGS_ 15);
    else
      opeu_open_delay(OPEN_ARGS_ 2);
  }
  opeu_open_delay(OPEN_ARGS_ 60);
  PICTURE_AT(0x110, 0x148, 0x63a2cL);
  if (opeu_open_wait(OPEN_ARGS_ 0x103)) return 1;
  if (opeu_open_wait_until(OPEN_ARGS_ 0xdced)) return 1;
  MUSIC_COMMAND4(0);
  while (MUSIC_PLAYING()) {
    if (INPUT_POLL()) return 1;
  }
  if (FADE_OUT()) return 1;
  opeu_open_clear_screen(OPEN_ARGS);
  return 0;
}

void opeu_open_init(OPEN_STATE) {
  int ems, handle;
  if (SET_BUFFER_G(DOS_ALLOC(0x501)) == 0) {
    CURSOR_PRINT(ALLOC_ERR1);
    HANG();
  }
  if (SET_BUFFER_H(DOS_ALLOC(0x6d5)) == 0) {
    CURSOR_PRINT(ALLOC_ERR2);
    HANG();
  }
  if (SET_BUFFER_A(DOS_ALLOC(0x1000)) == 0) {
    CURSOR_PRINT(ALLOC_ERR3);
    HANG();
  }
  if (SET_BUFFER_B(DOS_ALLOC(0x1000)) == 0) {
    CURSOR_PRINT(ALLOC_ERR4);
    HANG();
  }
  if (SET_BUFFER_C(DOS_ALLOC(0x1000)) == 0) {
    CURSOR_PRINT(ALLOC_ERR5);
    HANG();
  }
  if (SET_BUFFER_D(DOS_ALLOC(0x1000)) == 0) {
    CURSOR_PRINT(ALLOC_ERR6);
    HANG();
  }
  if (SET_BUFFER_E(DOS_ALLOC(0x1000)) == 0) {
    CURSOR_PRINT(ALLOC_ERR7);
    HANG();
  }
  ems = EMS_DETECT();
  if (SET_BUFFER_F(DOS_ALLOC(0x1000)) == 0 && (!ems || !EMS_ALLOC_SPRITE())) {
    CURSOR_PRINT(ALLOC_ERR9);
    HANG();
  }
  handle = FILE_OR_DIE(NAME_MOPENGRP);
  DOS_READ_FAR(handle, BUFFER_H, 0, 0x6d40);
  DOS_CLOSE(handle);
  STUB_09B7(1, 0);
  VGA_CLEAR();
  UI_TEXT(TEXT_CONSOLE);
  IDENTITY_PALETTE();
  SOUND_INITIALIZE();
  MOUSE_RESET();
}

void opeu_open_free_buffers(OPEN_STATE) {
  VGA_CLEAR();
  DOS_FREE(BUFFER_G);
  DOS_FREE(BUFFER_H);
  DOS_FREE(BUFFER_A);
  DOS_FREE(BUFFER_B);
  DOS_FREE(BUFFER_C);
  DOS_FREE(BUFFER_D);
  DOS_FREE(BUFFER_E);
  if (!EMS_USED) DOS_FREE(BUFFER_F);
  MEMORY_RELEASE();
}

/* Music driver (int 66h): AH=1 AL=5, then AH=2 AL=20. */
void opeu_open_shutdown(OPEN_STATE) {
  Regs regs;
  FADE_OUT_WAIT();
  regs.h.ah = 1;
  regs.h.al = 5;
  INT86(0x66, &regs, &regs);
  regs.h.ah = 2;
  regs.h.al = 20;
  INT86(0x66, &regs, &regs);
  VGA_CLEAR();
  FADE_ALT();
  while (!DOS_EXISTS(NAME_SCESEL));
  opeu_open_free_buffers(OPEN_ARGS);
}

void opeu_open_files_open(OPEN_STATE) {
  SET_NPK_FILE(FILE_OR_DIE(NAME_NPK));
  SET_LOGO_FILE(FILE_OR_DIE(NAME_FNT));
}

void opeu_open_files_close(OPEN_STATE) {
  DOS_CLOSE(NPK_FILE);
  DOS_CLOSE(LOGO_FILE);
}

void opeu_open_main(OPEN_STATE) {
  INSTALL_HANDLERS();
  opeu_open_init(OPEN_ARGS);
  opeu_open_files_open(OPEN_ARGS);
  for (;;) {
    TIMELINE_START(20000);
    opeu_open_scene_koei(OPEN_ARGS);
    if (opeu_open_scene_prologue(OPEN_ARGS)) {
      SET_ABORTED(1);
      break;
    }
    if (opeu_open_scene_color_cycle(OPEN_ARGS)) {
      SET_ABORTED(1);
      break;
    }
    if (opeu_open_scene_flicker(OPEN_ARGS)) {
      SET_ABORTED(1);
      break;
    }
    if (opeu_open_scene_panorama(OPEN_ARGS)) {
      SET_ABORTED(1);
      break;
    }
    if (opeu_open_scene_units(OPEN_ARGS)) {
      SET_ABORTED(1);
      break;
    }
    if (opeu_open_scene_bombing(OPEN_ARGS)) {
      SET_ABORTED(1);
      break;
    }
    if (opeu_open_scene_flak(OPEN_ARGS)) {
      SET_ABORTED(1);
      break;
    }
    if (opeu_open_scene_artillery(OPEN_ARGS)) {
      SET_ABORTED(1);
      break;
    }
    if (opeu_open_title(OPEN_ARGS)) {
      SET_ABORTED(0);
      break;
    }
  }
  if (ABORTED) opeu_open_title(OPEN_ARGS);
  opeu_open_files_close(OPEN_ARGS);
  opeu_open_shutdown(OPEN_ARGS);
  FM_NOOP();
}
