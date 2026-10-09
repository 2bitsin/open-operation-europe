/* OPEN 03ac:0004..2817 (image 0x3ac4..0x62d7): the opening program, one code
 * segment.
 *
 * First half (03ac:0004..0e7b): timing helpers, the KOEI logo cards, the
 * prologue, the colour cycle and flicker/dissolve scenes and the panorama
 * scene. Timing: counter A (int 66h F001/F000) drives the uninterruptible
 * opeu_open_delay, which only waits when ticks != 0; counter B (int 66h
 * F100) is the music timeline, counting down, which opeu_open_wait
 * (relative) and opeu_open_wait_until (absolute) poll together with the
 * input (key or mouse button -> abort, result 1). Every abortable scene
 * returns 1 when aborted, 0 when it completed. Pictures come from
 * OPENGRP.NPK by absolute file position; captures of the screen go to the
 * six 64 KB far buffers whose segments are DS:1c22..1c2c; palettes are
 * 48-byte rows of the table at DS:0444.
 *
 * Second half (03ac:0e7c..2817): units scene: two vehicles drive in on
 * 2-byte strips, then play the eleven 160x192 frames listed at
 * DS:01ae/01c4/01da (frames 8-10 loop). Bombing: a bobbing bomber (bob table
 * DS:01f0, rotating 8x64 part) fades in over 58 frames synchronised to the
 * music timeline; three explosions are composed in far buffers (copy, 1-bpp
 * mask AND, sprite OR) and played with palette flashes. Flak: three flashes
 * over picture 0x54d6d, then the bomber again (DS:0250) with four flak
 * bursts until the palette is black. Artillery: ground and gun pictures are
 * captured in columns and panned in, then the gun fires shells along the
 * DS:028e..02d6 tables with a muzzle flash flicker (DS:02e2 on/off, DS:02e4
 * phase). Title: the colour-cycled title (short form after an abort,
 * DS:01ac), then the wait for the music to end. Init/shutdown allocate and
 * free the far buffers (DS:1c22..1c32; EMS page frame as the fallback for
 * 1c2c), read MOPENGRP.DAT, open and close OPENGRP.NPK/KOEILOGO.FNT, and wait
 * for SCESEL.DAT before leaving. opeu_open_main is the program's main(): it
 * repeats the opening until a scene is aborted (then shows the short title)
 * or the title is skipped.
 *
 * Every routine outside this segment is a service (OpenEventId). Near
 * pointers are DS offsets; all arithmetic wraps at 16 bits.
 */
#include "recovered.hpp"

namespace opeu::intro::detail::openmain {
namespace {
using enum OpenEventId;

auto open_call(uint8_t* ds, OpenEvent event, OpenEventId id, uint16_t count, uint16_t a0, uint16_t a1, uint16_t a2,
               uint16_t a3, uint16_t a4, uint16_t a5, uint16_t a6) -> uint16_t {
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

/* One service call with its argument words. */
template <class... Words>
auto Call(uint8_t* ds, OpenEvent event, OpenEventId id, Words... words) -> uint16_t {
  uint16_t const padded[7]{ static_cast<uint16_t>(words)... };
  return open_call(ds, event, id, sizeof...(Words), padded[0], padded[1], padded[2], padded[3], padded[4], padded[5],
                   padded[6]);
}

/* The services whose arguments are converted on the way. */
auto PictureAt(uint8_t* ds, OpenEvent event, uint16_t x, uint16_t y, uint32_t position) -> uint16_t {
  return Call(ds, event, PICTURE_AT, x, y, position, position >> 16);
}
auto DosSeek(uint8_t* ds, OpenEvent event, uint16_t handle, uint32_t position, uint16_t origin) -> uint16_t {
  return Call(ds, event, DOS_SEEK, handle, position, position >> 16, origin);
}
auto MusicStart(uint8_t* ds, OpenEvent event, uint8_t track) -> uint16_t {
  return Call(ds, event, MUSIC_START, track);
}
auto MusicCommand4(uint8_t* ds, OpenEvent event, uint8_t value) -> uint16_t {
  return Call(ds, event, MUSIC_COMMAND4, value);
}
auto Int86(uint8_t* ds, OpenEvent event, uint16_t number, Regs const& in) -> uint16_t {
  return Call(ds, event, INT86, number, in.h.al | in.h.ah << 8);
}

/* The near pointer of palette n. */
auto Palette(uint8_t* ds, unsigned n) -> uint16_t {
  return Near(ds, Data(ds).palettes[n]);
}

/* The allocation failures stop the program in an empty loop; the host
 * pumps its events instead (a loop without side effects is undefined in
 * C++), so the window still closes and a headless run still stops. */
auto Hang(uint8_t* ds, OpenEvent event) -> void {
  for (;;) Call(ds, event, HANG);
}
}

int opeu_open_wait(uint8_t* ds, OpenEvent event, uint16_t ticks);
int opeu_open_wait_until(uint8_t* ds, OpenEvent event, uint16_t target);
void opeu_open_clear_screen(uint8_t* ds, OpenEvent event);
int opeu_open_fade_to_palette(uint8_t* ds, OpenEvent event, uint8_t slot, int mode);
void opeu_open_load_koei_logo(uint8_t* ds, OpenEvent event);
void opeu_open_show_koei_logo(uint8_t* ds, OpenEvent event);
void opeu_open_show_logo_caption_a(uint8_t* ds, OpenEvent event);
void opeu_open_show_logo_caption_b(uint8_t* ds, OpenEvent event);
void opeu_open_scene_koei(uint8_t* ds, OpenEvent event);
int opeu_open_prologue_pictures(uint8_t* ds, OpenEvent event);
int opeu_open_scene_prologue(uint8_t* ds, OpenEvent event);
int opeu_open_descend_animation(uint8_t* ds, OpenEvent event);
int opeu_open_scene_color_cycle(uint8_t* ds, OpenEvent event);
int opeu_open_scene_flicker(uint8_t* ds, OpenEvent event);
int opeu_open_sprite_march(uint8_t* ds, OpenEvent event);
int opeu_open_panorama_scroll(uint8_t* ds, OpenEvent event);
int opeu_open_panorama_flashes(uint8_t* ds, OpenEvent event);
int opeu_open_panorama_exit(uint8_t* ds, OpenEvent event);
int opeu_open_scene_panorama(uint8_t* ds, OpenEvent event);
int opeu_open_units_drive_in(uint8_t* ds, OpenEvent event);
int opeu_open_units_animate(uint8_t* ds, OpenEvent event);
int opeu_open_scene_units(uint8_t* ds, OpenEvent event);
int opeu_open_bomber_approach(uint8_t* ds, OpenEvent event);
int opeu_open_bomb_explosions(uint8_t* ds, OpenEvent event);
int opeu_open_scene_bombing(uint8_t* ds, OpenEvent event);
int opeu_open_flash_preload(uint8_t* ds, OpenEvent event);
int opeu_open_bomber_flak(uint8_t* ds, OpenEvent event);
int opeu_open_scene_flak(uint8_t* ds, OpenEvent event);
int opeu_open_gun_setup(uint8_t* ds, OpenEvent event);
int opeu_open_gun_pan(uint8_t* ds, OpenEvent event);
void opeu_open_flash_toggle(uint8_t* ds, OpenEvent event);
int opeu_open_shell_run(uint8_t* ds, OpenEvent event, int first);
int opeu_open_shell_run_pair(uint8_t* ds, OpenEvent event);
int opeu_open_gun_fire(uint8_t* ds, OpenEvent event);
int opeu_open_scene_artillery(uint8_t* ds, OpenEvent event);
int opeu_open_title(uint8_t* ds, OpenEvent event);
void opeu_open_init(uint8_t* ds, OpenEvent event);
void opeu_open_free_buffers(uint8_t* ds, OpenEvent event);
void opeu_open_shutdown(uint8_t* ds, OpenEvent event);
void opeu_open_files_open(uint8_t* ds, OpenEvent event);
void opeu_open_files_close(uint8_t* ds, OpenEvent event);

void opeu_open_delay(uint8_t* ds, OpenEvent event, uint16_t ticks) {
  if (ticks) {
    Call(ds, event, SFX_PLAY, ticks);
    while (Call(ds, event, SFX_BUSY));
  }
}

int opeu_open_wait(uint8_t* ds, OpenEvent event, uint16_t ticks) {
  uint16_t target;
  target = Call(ds, event, FM_BUSY);
  target -= ticks;
  do {
    if (Call(ds, event, INPUT_POLL)) return 1;
  } while (Call(ds, event, FM_BUSY) > target);
  return 0;
}

int opeu_open_wait_until(uint8_t* ds, OpenEvent event, uint16_t target) {
  while (Call(ds, event, FM_BUSY) > target) {
    if (Call(ds, event, INPUT_POLL)) return 1;
  }
  return 0;
}

void opeu_open_clear_screen(uint8_t* ds, OpenEvent event) {
  Call(ds, event, VGA_CLEAR);
}

int opeu_open_fade_to_palette(uint8_t* ds, OpenEvent event, uint8_t slot, int mode) {
  return Call(ds, event, PALETTE_FADE, mode, Palette(ds, slot));
}

void opeu_open_load_koei_logo(uint8_t* ds, OpenEvent event) {
  DosSeek(ds, event, Data(ds).logo_file, 0L, 0);
  Call(ds, event, DOS_READ_FAR, Data(ds).logo_file, Data(ds).buffer_a, 0, 0xb60);
  DosSeek(ds, event, Data(ds).logo_file, 0x1860L, 0);
  Call(ds, event, DOS_READ_FAR, Data(ds).logo_file, Data(ds).buffer_a, 0xb60, 0x270);
}

void opeu_open_show_koei_logo(uint8_t* ds, OpenEvent event) {
  opeu_open_clear_screen(ds, event);
  opeu_open_fade_to_palette(ds, event, 2, -1);
  Call(ds, event, DRAW_FAR, 0x1b, 0x60, 0x1a, 0x70, 2, Data(ds).buffer_a, 0);
  opeu_open_delay(ds, event, 60);
  Call(ds, event, DRAW_FAR, 0x1b, 0xe0, 0x1a, 0x18, 7, Data(ds).buffer_a, 0xb60);
  opeu_open_delay(ds, event, 60);
  opeu_open_wait_until(ds, event, 0xe98a);
  Call(ds, event, FADE_OUT_WAIT);
}

void opeu_open_show_logo_caption_a(uint8_t* ds, OpenEvent event) {
  opeu_open_clear_screen(ds, event);
  opeu_open_fade_to_palette(ds, event, 2, -1);
  PictureAt(ds, event, 0x80, 0xbc, 0L);
  opeu_open_delay(ds, event, 0x78);
  opeu_open_wait_until(ds, event, 0xe868);
  Call(ds, event, FADE_OUT_WAIT);
}

void opeu_open_show_logo_caption_b(uint8_t* ds, OpenEvent event) {
  opeu_open_clear_screen(ds, event);
  opeu_open_fade_to_palette(ds, event, 2, -1);
  PictureAt(ds, event, 0xf2, 0x76, 0x7f0L);
  PictureAt(ds, event, 0x80, 0xba, 0x42dL);
  opeu_open_delay(ds, event, 0x78);
  opeu_open_wait_until(ds, event, 0xe50c);
  Call(ds, event, FADE_OUT_WAIT);
}

void opeu_open_scene_koei(uint8_t* ds, OpenEvent event) {
  opeu_open_load_koei_logo(ds, event);
  opeu_open_show_koei_logo(ds, event);
  opeu_open_show_logo_caption_a(ds, event);
  opeu_open_show_logo_caption_b(ds, event);
}

int opeu_open_prologue_pictures(uint8_t* ds, OpenEvent event) {
  opeu_open_clear_screen(ds, event);
  PictureAt(ds, event, 0, 0, 0x2955fL);
  Call(ds, event, GFX_WAIT);
  Call(ds, event, BITMAP_SAVE, 0, 0, 0x50, 0x8c, Data(ds).buffer_e, 0);
  Call(ds, event, BITMAP_SAVE, 0, 0x8c, 0x50, 0x8c, Data(ds).buffer_f, 0);
  opeu_open_clear_screen(ds, event);
  PictureAt(ds, event, 0x58, 0, 0xa81L);
  if (opeu_open_fade_to_palette(ds, event, 3, 2)) return 1;
  Call(ds, event, BITMAP_SAVE, 10, 0, 0x3c, 0xc8, Data(ds).buffer_b, 0);
  Call(ds, event, BITMAP_SAVE, 10, 0xc8, 0x3c, 0xc8, Data(ds).buffer_c, 0);
  if (opeu_open_wait(ds, event, 0x105)) return 1;
  if (opeu_open_wait_until(ds, event, 0xe25d)) return 1;
  if (Call(ds, event, FADE_OUT)) return 1;
  return 0;
}

int opeu_open_scene_prologue(uint8_t* ds, OpenEvent event) {
  if (opeu_open_wait_until(ds, event, 0xe4bf)) return 1;
  MusicStart(ds, event, 0);
  MusicCommand4(ds, event, 1);
  if (opeu_open_prologue_pictures(ds, event)) return 1;
  opeu_open_clear_screen(ds, event);
  PictureAt(ds, event, 0x90, 0x3c, 0x93f7L);
  if (opeu_open_fade_to_palette(ds, event, 4, 2)) return 1;
  Call(ds, event, BITMAP_SAVE, 0x12, 0x3c, 0x30, 0x154, Data(ds).buffer_d, 0);
  if (opeu_open_wait(ds, event, 0xdc)) return 1;
  if (opeu_open_wait_until(ds, event, 0xdfa9)) return 1;
  if (Call(ds, event, FADE_OUT)) return 1;
  return 0;
}

int opeu_open_descend_animation(uint8_t* ds, OpenEvent event) {
  uint16_t h;
  int i;
  uint16_t off_a;
  uint16_t off_b;
  opeu_open_clear_screen(ds, event);
  if (opeu_open_fade_to_palette(ds, event, 30, 2)) return 1;
  h = 0;
  off_a = 0x2580;
  off_b = 0x4a40;
  for (i = 0; i < 25; i++) {
    Call(ds, event, BLIT_FAR, 0x22, 0, 12, h += 2, 0, Data(ds).buffer_a, (uint16_t)(off_a -= 0x60));
    if (opeu_open_wait(ds, event, 6)) return 1;
    if (i > 21) Call(ds, event, PALETTE_STEP, Palette(ds, 0));
    Call(ds, event, BLIT_FAR, 0x22, 0, 12, h += 2, 0, Data(ds).buffer_a, (uint16_t)(off_a -= 0x60));
    if (opeu_open_wait(ds, event, 6)) return 1;
    if (i > 21) Call(ds, event, PALETTE_STEP, Palette(ds, 0));
    off_a -= 0xc0;
    Call(ds, event, BLIT_FAR, 0x22, 0, 12, h += 2, 0, Data(ds).buffer_a, (uint16_t)(off_b -= 0x60));
    if (opeu_open_wait(ds, event, 6)) return 1;
    if (i > 21) Call(ds, event, PALETTE_STEP, Palette(ds, 0));
    Call(ds, event, BLIT_FAR, 0x22, 0, 12, h += 2, 0, Data(ds).buffer_a, (uint16_t)(off_b -= 0x60));
    if (opeu_open_wait(ds, event, 6)) return 1;
    if (i > 21) Call(ds, event, PALETTE_STEP, Palette(ds, 0));
    off_b -= 0xc0;
  }
  if (Call(ds, event, FADE_OUT)) return 1;
  return 0;
}

int opeu_open_scene_color_cycle(uint8_t* ds, OpenEvent event) {
  uint8_t pal;
  int j;
  opeu_open_clear_screen(ds, event);
  PictureAt(ds, event, 0x58, 0, 0x121a0L);
  if (opeu_open_fade_to_palette(ds, event, 5, 2)) return 1;
  if (opeu_open_wait(ds, event, 0x103)) return 1;
  if (opeu_open_wait_until(ds, event, 0xdced)) return 1;
  Call(ds, event, PALETTE_SET, Palette(ds, 6));
  if (opeu_open_wait(ds, event, 5)) return 1;
  Call(ds, event, PALETTE_SET, Palette(ds, 7));
  if (opeu_open_wait(ds, event, 5)) return 1;
  Call(ds, event, PALETTE_SET, Palette(ds, 8));
  if (opeu_open_wait(ds, event, 5)) return 1;
  Call(ds, event, PALETTE_SET, Palette(ds, 9));
  if (opeu_open_wait(ds, event, 5)) return 1;
  Call(ds, event, PALETTE_SET, Palette(ds, 5));
  if (opeu_open_wait(ds, event, 0x14)) return 1;
  if (Call(ds, event, FADE_OUT)) return 1;
  opeu_open_clear_screen(ds, event);
  PictureAt(ds, event, 0, 0, 0x28343L);
  PictureAt(ds, event, 0x60, 0, 0x28c1bL);
  Call(ds, event, BITMAP_SAVE, 0, 0, 12, 0xc8, Data(ds).buffer_a, 0);
  Call(ds, event, BITMAP_SAVE, 12, 0, 12, 0xc8, Data(ds).buffer_a, 0x2580);
  opeu_open_clear_screen(ds, event);
  PictureAt(ds, event, 0, 0, 0x27b7dL);
  for (pal = 11, j = 0; j < 19; j++, pal++) {
    Call(ds, event, PALETTE_SET, Palette(ds, pal));
    if (opeu_open_wait(ds, event, 5)) return 1;
  }
  if (opeu_open_fade_to_palette(ds, event, 0, 1)) return 1;
  if (opeu_open_descend_animation(ds, event)) return 1;
  return 0;
}

int opeu_open_scene_flicker(uint8_t* ds, OpenEvent event) {
  int i, off;
  opeu_open_clear_screen(ds, event);
  Call(ds, event, GFX_WAIT);
  Call(ds, event, BLIT_FAR, 0, 0x3c, 0x50, 0x8c, 0, Data(ds).buffer_e, 0);
  Call(ds, event, BLIT_FAR, 0, 0xc8, 0x50, 0x8c, 0, Data(ds).buffer_f, 0);
  if (opeu_open_fade_to_palette(ds, event, 31, 2)) return 1;
  for (i = 0; i < 2; i++) {
    Call(ds, event, PALETTE_SET, Palette(ds, 32));
    if (opeu_open_wait(ds, event, 6)) return 1;
    Call(ds, event, PALETTE_SET, Palette(ds, 31));
    if (opeu_open_wait(ds, event, 6)) return 1;
  }
  for (i = 0; i < 10; i++) {
    Call(ds, event, PALETTE_SET, Palette(ds, 32));
    if (opeu_open_wait(ds, event, 5)) return 1;
    Call(ds, event, PALETTE_SET, Palette(ds, 31));
    if (opeu_open_wait(ds, event, 8)) return 1;
  }
  if (opeu_open_wait(ds, event, 0x78)) return 1;
  if (Call(ds, event, FADE_OUT)) return 1;
  opeu_open_clear_screen(ds, event);
  PictureAt(ds, event, 0, 0, 0x399b9L);
  Call(ds, event, BITMAP_SAVE, 0, 0, 0x12, 0x78, Data(ds).buffer_a, 0);
  Call(ds, event, BITMAP_SAVE, 0x12, 0, 0x12, 0x78, Data(ds).buffer_a, 0x21c0);
  Call(ds, event, BITMAP_SAVE, 0x24, 0, 0x12, 0x78, Data(ds).buffer_a, 0x4380);
  Call(ds, event, BITMAP_SAVE, 0, 0x78, 0x12, 0x78, Data(ds).buffer_a, 0x6540);
  Call(ds, event, BITMAP_SAVE, 0x12, 0x78, 0x12, 0x78, Data(ds).buffer_a, 0x8700);
  Call(ds, event, BITMAP_SAVE, 0x24, 0x78, 0x12, 0x78, Data(ds).buffer_a, 0xa8c0);
  opeu_open_clear_screen(ds, event);
  Call(ds, event, BLIT_FAR, 0x12, 0x3c, 0x30, 0x154, 0, Data(ds).buffer_d, 0);
  if (opeu_open_wait(ds, event, 0x3c)) return 1;
  PictureAt(ds, event, 0xc, 0x3c, 0x37110L);
  Call(ds, event, BITMAP_SAVE, 0, 0x3c, 0x26, 0x120, Data(ds).buffer_e, 0);
  opeu_open_clear_screen(ds, event);
  Call(ds, event, BLIT_FAR, 0x12, 0x3c, 0x30, 0x154, 0, Data(ds).buffer_d, 0);
  if (opeu_open_fade_to_palette(ds, event, 4, 2)) return 1;
  if (opeu_open_wait_until(ds, event, 0xd56a)) return 1;
  Call(ds, event, DISSOLVE, 0, 0x3c, Data(ds).buffer_e, 0, 0x0001);
  Call(ds, event, DISSOLVE, 0, 0x3c, Data(ds).buffer_e, 0, 0x0101);
  Call(ds, event, DISSOLVE, 0, 0x3c, Data(ds).buffer_e, 0, 0x1111);
  Call(ds, event, DISSOLVE, 0, 0x3c, Data(ds).buffer_e, 0, 0x5555);
  Call(ds, event, DISSOLVE, 0, 0x3c, Data(ds).buffer_e, 0, 0x7777);
  Call(ds, event, DISSOLVE, 0, 0x3c, Data(ds).buffer_e, 0, 0x7f7f);
  Call(ds, event, DISSOLVE, 0, 0x3c, Data(ds).buffer_e, 0, 0xffff);
  if (opeu_open_wait(ds, event, 0xc8)) return 1;
  if (opeu_open_wait_until(ds, event, 0xd3a2)) return 1;
  if (Call(ds, event, FADE_OUT)) return 1;
  opeu_open_clear_screen(ds, event);
  PictureAt(ds, event, 0, 0, 0x3e98bL);
  for (off = 0, i = 78; i >= 0; i--) {
    Call(ds, event, BITMAP_SAVE, i, 0, 1, 0xb0, Data(ds).buffer_d, (uint16_t)off);
    off += 0x2c0;
  }
  return 0;
}

int opeu_open_sprite_march(uint8_t* ds, OpenEvent event) {
  uint16_t d;
  int pass, j;
  uint16_t off_a;
  uint16_t off_b;
  opeu_open_clear_screen(ds, event);
  Call(ds, event, BLIT_FAR, 10, 0, 0x3c, 0xc8, 0, Data(ds).buffer_b, 0);
  Call(ds, event, BLIT_FAR, 10, 0xc8, 0x3c, 0xc8, 0, Data(ds).buffer_c, 0);
  if (opeu_open_fade_to_palette(ds, event, 3, 2)) return 1;
  if (opeu_open_wait_until(ds, event, 0xd2ed)) return 1;
  for (d = 0x40, pass = 0;; d >>= 1, d++, pass++) {
    for (off_a = 0, off_b = 0x6540, j = 0; j < 3; j++) {
      Call(ds, event, BLIT_FAR, 10, 0x118, 0x12, 0x78, 0, Data(ds).buffer_a, (uint16_t)off_a);
      Call(ds, event, BLIT_FAR, 0x34, 0x118, 0x12, 0x78, 0, Data(ds).buffer_a, (uint16_t)off_b);
      off_a += 0x21c0;
      off_b += 0x21c0;
      if (opeu_open_wait(ds, event, d)) return 1;
    }
    if (pass > 40 && !Call(ds, event, PALETTE_STEP, Palette(ds, 0))) {
      if (Call(ds, event, FADE_OUT)) return 1;
      break;
    }
  }
  return 0;
}

int opeu_open_panorama_scroll(uint8_t* ds, OpenEvent event) {
  int i;
  uint16_t off;
  opeu_open_clear_screen(ds, event);
  PictureAt(ds, event, 0, 0, 0x40bd5L);
  Call(ds, event, BITMAP_SAVE, 0, 0, 0x20, 0xb0, Data(ds).buffer_a, 0);
  if (opeu_open_wait(ds, event, 1)) return 1;
  PictureAt(ds, event, 0, 0, 0x421d5L);
  Call(ds, event, BITMAP_SAVE, 0, 0, 0x20, 0xb0, Data(ds).buffer_b, 0);
  if (opeu_open_wait(ds, event, 1)) return 1;
  PictureAt(ds, event, 0, 0, 0x43202L);
  Call(ds, event, BITMAP_SAVE, 0, 0, 0x20, 0xb0, Data(ds).buffer_c, 0);
  opeu_open_clear_screen(ds, event);
  if (opeu_open_fade_to_palette(ds, event, 33, 0)) return 1;
  for (off = 0, i = 0; i < 0x4f; i++) {
    Call(ds, event, SCREEN_COPY, 0x19, 0x70, 0x1d, 0xb0, 0x1a, 0x70);
    Call(ds, event, BLIT_FAR, 0x19, 0x70, 1, 0xb0, 0, Data(ds).buffer_d, (uint16_t)off);
    if (opeu_open_wait(ds, event, 5)) return 1;
    off += 0x2c0;
  }
  Call(ds, event, BITMAP_SAVE, 0x19, 0x70, 0x20, 0xb0, Data(ds).buffer_d, 0);
  if (opeu_open_wait_until(ds, event, 0xcc15)) return 1;
  return 0;
}

int opeu_open_panorama_flashes(uint8_t* ds, OpenEvent event) {
  int i;
  for (i = 0; i < 2; i++) {
    Call(ds, event, PALETTE_SET, Palette(ds, 34));
    Call(ds, event, PALETTE_SET, Palette(ds, 35));
    if (opeu_open_wait(ds, event, 2)) return 1;
    Call(ds, event, BLIT_FAR, 0x18, 0x70, 0x20, 0xb0, 0, Data(ds).buffer_a, 0);
    Call(ds, event, PALETTE_SET, Palette(ds, 33));
    if (opeu_open_wait(ds, event, 10)) return 1;
    Call(ds, event, BLIT_FAR, 0x18, 0x70, 0x20, 0xb0, 0, Data(ds).buffer_b, 0);
    if (opeu_open_wait(ds, event, 10)) return 1;
    Call(ds, event, BLIT_FAR, 0x18, 0x70, 0x20, 0xb0, 0, Data(ds).buffer_c, 0);
    if (opeu_open_wait(ds, event, 10)) return 1;
    Call(ds, event, BLIT_FAR, 0x18, 0x70, 0x20, 0xb0, 0, Data(ds).buffer_d, 0);
    if (i == 0 && opeu_open_wait(ds, event, 100)) return 1;
  }
  if (opeu_open_wait(ds, event, 0x1e)) return 1;
  if (opeu_open_wait_until(ds, event, 0xc9f8)) return 1;
  return 0;
}

int opeu_open_panorama_exit(uint8_t* ds, OpenEvent event) {
  int i;
  Call(ds, event, UI_BOX, 0x38, 0x70, 1, 0xb0, 0);
  for (i = 0; i < 15; i++) {
    Call(ds, event, SCREEN_COPY, 0x17, 0x70, 0x1f, 0xb0, 0x18, 0x70);
    if (opeu_open_wait(ds, event, 5)) return 1;
  }
  if (opeu_open_wait_until(ds, event, 0xc99e)) return 1;
  if (Call(ds, event, FADE_OUT)) return 1;
  PictureAt(ds, event, 0, 0, 0x43fd2L);
  Call(ds, event, BITMAP_SAVE, 0, 0, 0x14, 0xc0, Data(ds).buffer_a, 0);
  Call(ds, event, BITMAP_SAVE, 0x14, 0, 0x14, 0xc0, Data(ds).buffer_a, 0x3c00);
  Call(ds, event, BITMAP_SAVE, 0x28, 0, 0x14, 0xc0, Data(ds).buffer_a, 0x7800);
  Call(ds, event, BITMAP_SAVE, 0x3c, 0, 0x14, 0xc0, Data(ds).buffer_a, 0xb400);
  Call(ds, event, BITMAP_SAVE, 0, 0xc0, 0x14, 0xc0, Data(ds).buffer_b, 0);
  Call(ds, event, BITMAP_SAVE, 0x14, 0xc0, 0x14, 0xc0, Data(ds).buffer_b, 0x3c00);
  Call(ds, event, BITMAP_SAVE, 0x28, 0xc0, 0x14, 0xc0, Data(ds).buffer_b, 0x7800);
  Call(ds, event, BITMAP_SAVE, 0x3c, 0xc0, 0x14, 0xc0, Data(ds).buffer_b, 0xb400);
  return 0;
}

int opeu_open_scene_panorama(uint8_t* ds, OpenEvent event) {
  if (opeu_open_sprite_march(ds, event)) return 1;
  if (opeu_open_panorama_scroll(ds, event)) return 1;
  if (opeu_open_panorama_flashes(ds, event)) return 1;
  if (opeu_open_panorama_exit(ds, event)) return 1;
  return 0;
}
/* x: strip column, off_a/off_b: strip offsets in buffer C. */
int opeu_open_units_drive_in(uint8_t* ds, OpenEvent event) {
  int x;
  uint16_t off_a, off_b;
  opeu_open_clear_screen(ds, event);
  Call(ds, event, BLIT_FAR, 2, 0, 0x14, 0x60, 0, Data(ds).buffer_a, 0);
  off_a = 0;
  for (x = 0x15; x > 0; x -= 2) {
    Call(ds, event, BITMAP_SAVE, x, 0, 2, 0x60, Data(ds).buffer_c, off_a);
    off_a += 0x300;
  }
  for (x = 0x14; x >= 0; x -= 2) {
    Call(ds, event, BITMAP_SAVE, x, 0, 2, 0x60, Data(ds).buffer_c, off_a);
    off_a += 0x300;
  }
  opeu_open_clear_screen(ds, event);
  Call(ds, event, PALETTE_SET, Palette(ds, 36));
  x = off_b = off_a = 0;
  for (; x < 31; x++) {
    Call(ds, event, SCREEN_COPY, 0, 0xc4, 0x3c, 0x60, 2, 0xc4);
    if (x < 11) {
      Call(ds, event, BLIT_FAR, 0, 0xc4, 2, 0x60, 0, Data(ds).buffer_c, off_a);
      off_a += 0x300;
    }
    if (x > 12 && x < 24) {
      Call(ds, event, BLIT_FAR, 0, 0xc4, 2, 0x60, 0, Data(ds).buffer_c, off_b);
      off_b += 0x300;
    }
    if (opeu_open_wait(ds, event, 6)) return 1;
  }
  if (opeu_open_wait(ds, event, 10)) return 1;
  return 0;
}

/* i: frame count, j/k: frame of the right/left vehicle. */
int opeu_open_units_animate(uint8_t* ds, OpenEvent event) {
  int i, j, k;
  i = k = j = 0;
  for (; i < 40; i++) {
    if (i == 0) {
      Call(ds, event, PALETTE_SET, Palette(ds, 34));
      Call(ds, event, PALETTE_SET, Palette(ds, 35));
      if (opeu_open_wait(ds, event, 2)) return 1;
      Call(ds, event, PALETTE_SET, Palette(ds, 36));
    }
    Call(ds, event, UI_BOX, 0x2a, 0x64, 0x14, 0x80, 0);
    Call(ds, event, BLIT_FAR, 0x2a, Data(ds).frame_y[j], 0x14, Data(ds).frame_h[j], 0,
         j < 5 ? Data(ds).buffer_a : Data(ds).buffer_b, Data(ds).frame_off[j]);
    j++;
    if (j == 11) j = 8;
    Call(ds, event, UI_BOX, 0x10, 0x64, 0x14, 0x80, 0);
    Call(ds, event, BLIT_FAR, 0x10, Data(ds).frame_y[k], 0x14, Data(ds).frame_h[k], 0,
         k < 5 ? Data(ds).buffer_a : Data(ds).buffer_b, Data(ds).frame_off[k]);
    if (i > 2) {
      k++;
      if (k == 11) k = 8;
    }
    if (opeu_open_wait(ds, event, 10)) return 1;
    if (i > 30) Call(ds, event, PALETTE_STEP, Palette(ds, 0));
  }
  if (Call(ds, event, FADE_OUT)) return 1;
  if (opeu_open_wait_until(ds, event, 0xc5e2)) return 1;
  return 0;
}

int opeu_open_scene_units(uint8_t* ds, OpenEvent event) {
  if (opeu_open_units_drive_in(ds, event)) return 1;
  if (opeu_open_units_animate(ds, event)) return 1;
  return 0;
}

/* a: composite toggle, b: frame deadline, c: bob index, d: frame,
 * e: y, f: faded in, g: rotating part offset, h: next y. */
int opeu_open_bomber_approach(uint8_t* ds, OpenEvent event) {
  uint16_t a;
  uint16_t b;
  int c, d;
  uint16_t e;
  int f;
  uint16_t g, h;
  opeu_open_clear_screen(ds, event);
  PictureAt(ds, event, 0, 0, 0x46848L);
  Call(ds, event, BITMAP_SAVE, 0, 0, 0x50, 0xa0, Data(ds).buffer_c, 0);
  opeu_open_clear_screen(ds, event);
  PictureAt(ds, event, 0, 0, 0x487fbL);
  Call(ds, event, BITMAP_SAVE, 0, 0, 8, 0x40, Data(ds).buffer_a, 0);
  Call(ds, event, BITMAP_SAVE, 8, 0, 8, 0x40, Data(ds).buffer_a, 0x800);
  Call(ds, event, BITMAP_SAVE, 0x10, 0, 8, 0x40, Data(ds).buffer_a, 0x1000);
  Call(ds, event, BITMAP_SAVE, 0x18, 0, 8, 0x40, Data(ds).buffer_a, 0x1800);
  Call(ds, event, BITMAP_SAVE, 0x20, 0, 8, 0x40, Data(ds).buffer_a, 0x2000);
  PictureAt(ds, event, 0, 0, 0x48ce9L);
  Call(ds, event, BITMAP_SAVE, 0, 0, 0x32, 0x140, Data(ds).buffer_b, 0);
  PictureAt(ds, event, 0, 0, 0x4d847L);
  Call(ds, event, BITMAP_SAVE, 0, 0, 0x1a, 0xb0, Data(ds).buffer_e, 0);
  Call(ds, event, BITMAP_SAVE, 0x1a, 0, 0x1a, 0xb0, Data(ds).buffer_e, 0x4780);
  Call(ds, event, BITMAP_SAVE, 0x34, 0, 0x1a, 0xb0, Data(ds).buffer_e, 0x8f00);
  Call(ds, event, GFX_WAIT);
  Call(ds, event, BITMAP_SAVE, 0, 0xb0, 0x1a, 0xb0, Data(ds).buffer_f, 0);
  Call(ds, event, BITMAP_SAVE, 0x1a, 0xb0, 0x1a, 0xb0, Data(ds).buffer_f, 0x4780);
  Call(ds, event, BITMAP_SAVE, 0x34, 0xb0, 0x1a, 0xb0, Data(ds).buffer_f, 0x8f00);
  opeu_open_clear_screen(ds, event);
  PictureAt(ds, event, 0, 0, 0x47428L);
  Call(ds, event, BITMAP_SAVE, 0, 0, 0x10, 0x90, Data(ds).buffer_a, 0x2800);
  Call(ds, event, BITMAP_SAVE, 0x10, 0, 0x10, 0x90, Data(ds).buffer_a, 0x7000);
  Call(ds, event, BITMAP_SAVE, 0x20, 0, 0x10, 0x90, Data(ds).buffer_a, 0x4c00);
  Call(ds, event, BITMAP_SAVE, 0x30, 0, 0x10, 0x90, Data(ds).buffer_a, 0x9400);
  opeu_open_clear_screen(ds, event);
  Call(ds, event, BLIT_FAR, 0, 0x50, 0x50, 0xa0, 0, Data(ds).buffer_c, 0);
  Call(ds, event, BLIT_FAR, 0x13, 0x50, 0x10, 0x90, 0, Data(ds).buffer_a, 0x2800);
  Call(ds, event, BLIT_FAR, 0x32, 0x50, 0x10, 0x90, 0, Data(ds).buffer_a, 0x4c00);
  Call(ds, event, BLIT_FAR, 0, 0xf0, 0x50, 0xa0, 0, Data(ds).buffer_c, 0);
  Call(ds, event, BLIT_FAR, 0x13, 0xf0, 0x10, 0x90, 0, Data(ds).buffer_a, 0x7000);
  Call(ds, event, BLIT_FAR, 0x32, 0xf0, 0x10, 0x90, 0, Data(ds).buffer_a, 0x9400);
  Call(ds, event, BITMAP_SAVE, 0, 0x50, 0x50, 0xa0, Data(ds).buffer_c, 0);
  Call(ds, event, BITMAP_SAVE, 0, 0xf0, 0x50, 0xa0, Data(ds).buffer_d, 0);
  opeu_open_clear_screen(ds, event);
  Call(ds, event, BLIT_FAR, 0, 0x50, 0x50, 0xa0, 0, Data(ds).buffer_d, 0);
  h = e = Data(ds).bob_a[0];
  d = f = c = a = g = 0;
  for (; d < 0x3a; d++) {
    b = Call(ds, event, FM_BUSY);
    b -= 12;
    e = Data(ds).bob_a[c];
    Call(ds, event, BLIT_FAR, 0, e, 0x50, 0xa0, 0, a ? Data(ds).buffer_d : Data(ds).buffer_c, 0);
    Call(ds, event, BLIT_FAR, 0x23, e + 0x60, 8, 0x40, 0, Data(ds).buffer_a, g);
    a ^= 1;
    c++;
    if (c > 19) c = 0;
    h = Data(ds).bob_a[c];
    if (h < e) Call(ds, event, UI_BOX, 0, h + 0xa0, 0x50, e - h, 0);
    g += 0x800;
    if (g >= 0x2800) g = 0;
    if (!f) {
      if (!Call(ds, event, PALETTE_STEP, Palette(ds, 0x25))) f = 1;
    }
    while (Call(ds, event, FM_BUSY) > b) {
      if (Call(ds, event, INPUT_POLL)) return 1;
    }
  }
  if (Call(ds, event, FADE_OUT)) return 1;
  return 0;
}

/* k: explosion, j: frame, n: palette step, pos: frame offset. */
int opeu_open_bomb_explosions(uint8_t* ds, OpenEvent event) {
  int k, j, n;
  uint16_t pos;
  opeu_open_clear_screen(ds, event);
  Call(ds, event, PALETTE_SET, Palette(ds, 0x26));
  Call(ds, event, BLIT_FAR, 0xf, 0x28, 0x32, 0x140, 0, Data(ds).buffer_b, 0);
  if (opeu_open_wait_until(ds, event, 0xbd4b)) return 1;
  if (opeu_open_wait(ds, event, 50)) return 1;
  for (k = 0; k < 3; k++) {
    Call(ds, event, BITMAP_SAVE, Data(ds).blast_x[k], Data(ds).blast_y[k], 0x1a, 0xb0, Data(ds).buffer_a, 0xb800);
    for (pos = 0, j = 0; j < 3; j++, pos += 0x4780) {
      Call(ds, event, FAR_COPY, Data(ds).buffer_a, 0xb800, Data(ds).buffer_b, pos, 0x4780);
      Call(ds, event, BUF_AND_MASK, Data(ds).buffer_b, pos, 0x1a, 0xb0, Data(ds).buffer_h, Data(ds).blast_masks[j]);
      Call(ds, event, BUF_OR, Data(ds).buffer_b, pos, 0x1a, 0xb0, Data(ds).buffer_e, Data(ds).blast_sprites[j]);
    }
    for (pos = 0, j = 3; j < 6; j++, pos += 0x4780) {
      Call(ds, event, FAR_COPY, Data(ds).buffer_a, 0xb800, Data(ds).buffer_c, pos, 0x4780);
      Call(ds, event, BUF_AND_MASK, Data(ds).buffer_c, pos, 0x1a, 0xb0, Data(ds).buffer_h, Data(ds).blast_masks[j]);
      Call(ds, event, GFX_WAIT);
      Call(ds, event, BUF_OR, Data(ds).buffer_c, pos, 0x1a, 0xb0, Data(ds).buffer_f, Data(ds).blast_sprites[j]);
    }
    for (pos = 0, j = 0; j < 6; j++) {
      Call(ds, event, BLIT_FAR, Data(ds).blast_x[k], Data(ds).blast_y[k], 0x1a, 0xb0, 4,
           j < 3 ? Data(ds).buffer_b : Data(ds).buffer_c, pos);
      if (pos == 0x8f00)
        pos = 0;
      else
        pos += 0x4780;
      for (n = 0; n < Data(ds).blast_steps[j]; n++) Call(ds, event, PALETTE_STEP, Palette(ds, Data(ds).blast_slots[k]));
      if (opeu_open_wait(ds, event, 2)) return 1;
    }
    Call(ds, event, BLIT_FAR, Data(ds).blast_x[k], Data(ds).blast_y[k], 0x1a, 0xb0, 0, Data(ds).buffer_a, 0xb800);
    if (opeu_open_fade_to_palette(ds, event, 0x26, 1)) return 1;
    Call(ds, event, BLIT_FAR, Data(ds).blast_x[k], Data(ds).blast_y[k], 0x1a, 0xb0, 0, Data(ds).buffer_a, 0xb800);
    if (opeu_open_wait(ds, event, 10)) return 1;
  }
  if (opeu_open_wait(ds, event, 90)) return 1;
  if (opeu_open_wait_until(ds, event, 0xbada)) return 1;
  if (Call(ds, event, FADE_OUT)) return 1;
  return 0;
}

int opeu_open_scene_bombing(uint8_t* ds, OpenEvent event) {
  if (opeu_open_bomber_approach(ds, event)) return 1;
  if (opeu_open_bomb_explosions(ds, event)) return 1;
  return 0;
}

int opeu_open_flash_preload(uint8_t* ds, OpenEvent event) {
  int i;
  opeu_open_clear_screen(ds, event);
  PictureAt(ds, event, 0x4c, 0xc, 0x54d6dL);
  if (opeu_open_fade_to_palette(ds, event, 0x2a, 2)) return 1;
  if (opeu_open_wait(ds, event, 100)) return 1;
  if (opeu_open_wait_until(ds, event, 0xb8d3)) return 1;
  for (i = 0; i < 3; i++) {
    Call(ds, event, PALETTE_SET, Palette(ds, Data(ds).flash_slots[i]));
    while (Call(ds, event, PALETTE_STEP, Palette(ds, 0x2a)));
    if (opeu_open_wait(ds, event, 60)) return 1;
  }
  if (opeu_open_wait(ds, event, 30)) return 1;
  if (opeu_open_wait_until(ds, event, 0xb6f8)) return 1;
  if (Call(ds, event, FADE_OUT)) return 1;
  opeu_open_clear_screen(ds, event);
  PictureAt(ds, event, 0, 0xa, 0x5b770L);
  Call(ds, event, BITMAP_SAVE, 0, 0, 6, 0x3c, Data(ds).buffer_b, 0);
  Call(ds, event, BITMAP_SAVE, 6, 0, 6, 0x3c, Data(ds).buffer_b, 0x5a0);
  Call(ds, event, BITMAP_SAVE, 0xc, 0, 6, 0x3c, Data(ds).buffer_b, 0xb40);
  Call(ds, event, BITMAP_SAVE, 0x12, 0, 6, 0x3c, Data(ds).buffer_b, 0x10e0);
  opeu_open_clear_screen(ds, event);
  PictureAt(ds, event, 0, 0, 0x5b9e3L);
  Call(ds, event, BITMAP_SAVE, 0, 0, 0x20, 0xe0, Data(ds).buffer_e, 0);
  PictureAt(ds, event, 0, 0, 0x5d486L);
  Call(ds, event, BITMAP_SAVE, 0, 0, 0x20, 0xe0, Data(ds).buffer_e, 0x7000);
  if (opeu_open_wait(ds, event, 15)) return 1;
  if (opeu_open_wait_until(ds, event, 0xb6f8)) return 1;
  if (Call(ds, event, FADE_OUT)) return 1;
  return 0;
}

/* a: composite toggle, b: frame % 10, d: deadline, e: bob index,
 * f: frame, g: y, h: rotating part offset, i: next y, j: burst offset. */
int opeu_open_bomber_flak(uint8_t* ds, OpenEvent event) {
  uint16_t a;
  int b;
  uint16_t d;
  int e, f;
  uint16_t g, h, i, j;
  opeu_open_clear_screen(ds, event);
  Call(ds, event, BLIT_FAR, 0, 0x50, 0x50, 0xa0, 0, Data(ds).buffer_d, 0);
  Call(ds, event, BLIT_FAR, 0x13, 0x50, 0x10, 0x90, 0, Data(ds).buffer_a, 0x2800);
  Call(ds, event, BLIT_FAR, 0x32, 0x50, 0x10, 0x90, 0, Data(ds).buffer_a, 0x4c00);
  Call(ds, event, BITMAP_SAVE, 0, 0x50, 0x50, 0xa0, Data(ds).buffer_c, 0);
  opeu_open_clear_screen(ds, event);
  Call(ds, event, PALETTE_SET, Palette(ds, 0x2e));
  Call(ds, event, BLIT_FAR, 0, 0x50, 0x50, 0xa0, 0, Data(ds).buffer_d, 0);
  i = g = Data(ds).bob_b[0];
  f = e = j = h = a = 0;
  for (;; f++) {
    d = Call(ds, event, FM_BUSY);
    d -= 12;
    g = Data(ds).bob_b[e];
    Call(ds, event, BLIT_FAR, 0, g, 0x50, 0xa0, 0, a ? Data(ds).buffer_d : Data(ds).buffer_c, 0);
    Call(ds, event, BLIT_FAR, 0x23, g + 0x60, 8, 0x40, 0, Data(ds).buffer_a, h);
    a ^= 1;
    e++;
    if (e > 19) e = 0;
    i = Data(ds).bob_b[e];
    if (i < g) Call(ds, event, UI_BOX, 0, i + 0xa0, 0x50, g - i, 0);
    b = f % 10;
    if (b > 4 && b < 9) {
      Call(ds, event, BLIT_FAR, Data(ds).flak_x[f / 10 % 4] - b + 5, Data(ds).flak_y[f / 10 % 4], 6, 0x3c, 0,
           Data(ds).buffer_b, j);
      j += 0x5a0;
      if (j >= 0x1680) j = 0;
    }
    h += 0x800;
    if (h >= 0x2800) h = 0;
    if (f >= 30) {
      if (!Call(ds, event, PALETTE_STEP, Palette(ds, 0))) break;
    }
    while (Call(ds, event, FM_BUSY) > d) {
      if (Call(ds, event, INPUT_POLL)) return 1;
    }
  }
  if (Call(ds, event, FADE_OUT)) return 1;
  return 0;
}

int opeu_open_scene_flak(uint8_t* ds, OpenEvent event) {
  if (opeu_open_flash_preload(ds, event)) return 1;
  if (opeu_open_bomber_flak(ds, event)) return 1;
  return 0;
}

/* p: column / flip count, toggle: figure frame, k: salvo, o: column offset. */
int opeu_open_gun_setup(uint8_t* ds, OpenEvent event) {
  int p, toggle, k;
  uint16_t o;
  opeu_open_clear_screen(ds, event);
  PictureAt(ds, event, 0, 0, 0x5eddeL);
  Call(ds, event, BITMAP_SAVE, 0, 0, 6, 0x30, Data(ds).buffer_a, 0);
  Call(ds, event, BITMAP_SAVE, 6, 0, 6, 0x30, Data(ds).buffer_a, 0x480);
  Call(ds, event, BITMAP_SAVE, 0xc, 0, 6, 0x30, Data(ds).buffer_a, 0x900);
  Call(ds, event, BITMAP_SAVE, 0x12, 0, 6, 0x30, Data(ds).buffer_a, 0xd80);
  opeu_open_clear_screen(ds, event);
  Call(ds, event, BLIT_FAR, 0, 0x108, 6, 0x30, 0, Data(ds).buffer_a, 0x900);
  Call(ds, event, BLIT_FAR, 6, 0x108, 6, 0x30, 0, Data(ds).buffer_a, 0x900);
  Call(ds, event, BLIT_FAR, 0xc, 0x108, 6, 0x30, 0, Data(ds).buffer_a, 0xd80);
  Call(ds, event, BLIT_FAR, 0x12, 0x108, 6, 0x30, 0, Data(ds).buffer_a, 0x480);
  Call(ds, event, BLIT_FAR, 0x18, 0x108, 6, 0x30, 0, Data(ds).buffer_a, 0x900);
  Call(ds, event, BLIT_FAR, 0x1e, 0x108, 6, 0x30, 0, Data(ds).buffer_a, 0x480);
  Call(ds, event, BLIT_FAR, 0x24, 0x108, 6, 0x30, 0, Data(ds).buffer_a, 0xd80);
  Call(ds, event, BLIT_FAR, 0x28, 0x108, 6, 0x30, 0, Data(ds).buffer_a, 0xd80);
  Call(ds, event, BLIT_FAR, 0x30, 0x108, 6, 0x30, 0, Data(ds).buffer_a, 0x900);
  Call(ds, event, BLIT_FAR, 0x36, 0x108, 6, 0x30, 0, Data(ds).buffer_a, 0);
  o = 0;
  for (p = 0x35; p >= 0; p--) {
    Call(ds, event, BITMAP_SAVE, p, 0x108, 1, 0x30, Data(ds).buffer_b, o);
    o += 0xc0;
  }
  opeu_open_clear_screen(ds, event);
  PictureAt(ds, event, 0, 0x88, 0x5f135L);
  if (opeu_open_wait(ds, event, 1)) return 1;
  Call(ds, event, BLIT_FAR, 0x28, 0x108, 6, 0x30, 0, Data(ds).buffer_a, 0x900);
  o = 0;
  for (p = 0x27; p >= 0; p--) {
    Call(ds, event, BITMAP_SAVE, p, 0x88, 1, 0xe0, Data(ds).buffer_c, o);
    o += 0x380;
  }
  opeu_open_clear_screen(ds, event);
  if (opeu_open_fade_to_palette(ds, event, 0x2f, 2)) return 1;
  Call(ds, event, BLIT_FAR, 0x18, 0x58, 0x20, 0xe0, 0, Data(ds).buffer_e, 0);
  Call(ds, event, BLIT_FAR, 0x12, 0x108, 6, 0x30, 0, Data(ds).buffer_a, 0);
  Call(ds, event, BLIT_FAR, 0xc, 0x108, 6, 0x30, 0, Data(ds).buffer_a, 0x900);
  Call(ds, event, BLIT_FAR, 6, 0x108, 6, 0x30, 0, Data(ds).buffer_a, 0x480);
  Call(ds, event, BLIT_FAR, 0, 0x108, 6, 0x30, 0, Data(ds).buffer_a, 0);
  Call(ds, event, BITMAP_SAVE, 1, 0x58, 0x36, 0xe0, Data(ds).buffer_d, 0);
  if (opeu_open_wait(ds, event, 100)) return 1;
  if (opeu_open_wait_until(ds, event, 0xb121)) return 1;
  for (k = toggle = 0; k < 3; k++) {
    for (p = 0; p < Data(ds).gun_flips[k]; p++) {
      Call(ds, event, BLIT_FAR, 0x18, 0x58, 0x20, 0xe0, 0, Data(ds).buffer_e, toggle ? 0 : 0x7000);
      if (opeu_open_wait(ds, event, toggle ? 2 : 4)) return 1;
      toggle ^= 1;
    }
    if (opeu_open_wait(ds, event, 100)) return 1;
  }
  return 0;
}

int opeu_open_gun_pan(uint8_t* ds, OpenEvent event) {
  int i;
  uint16_t off;
  for (off = 0, i = 0; i < 0x36; i++) {
    Call(ds, event, SCREEN_COPY, 0, 0x58, 0x36, 0xe0, 1, 0x58);
    Call(ds, event, BLIT_FAR, 0, 0x108, 1, 0x30, 0, Data(ds).buffer_b, off);
    off += 0xc0;
    if (Call(ds, event, INPUT_POLL)) return 1;
  }
  for (off = 0, i = 0; i < 0x28; i++) {
    Call(ds, event, SCREEN_COPY, 0, 0x58, 0x36, 0x110, 1, 0x58);
    Call(ds, event, BLIT_FAR, 0, 0x88, 1, 0xe0, 0, Data(ds).buffer_c, off);
    off += 0x380;
    if (Call(ds, event, INPUT_POLL)) return 1;
  }
  if (opeu_open_wait(ds, event, 15)) return 1;
  if (opeu_open_wait_until(ds, event, 0xac2d)) return 1;
  return 0;
}

void opeu_open_flash_toggle(uint8_t* ds, OpenEvent event) {
  if (Data(ds).flash_on) {
    if (Data(ds).flash_phase)
      Call(ds, event, PALETTE_SET, Palette(ds, 0x2f));
    else
      Call(ds, event, PALETTE_SET, Palette(ds, 0x30));
    Data(ds).flash_phase ^= 1;
  }
}

int opeu_open_shell_run(uint8_t* ds, OpenEvent event, int first) {
  int i;
  for (i = first; i < first + 3; i++) {
    Call(ds, event, DRAW_FAR, Data(ds).shell_x[i], Data(ds).shell_y[i], Data(ds).shell_w[i], Data(ds).shell_h[i], 0x407,
         Data(ds).buffer_h, Data(ds).shell_sprite[i]);
    if (opeu_open_wait(ds, event, 1)) return 1;
    Call(ds, event, BLIT_FAR, Data(ds).shell_x[i], Data(ds).shell_y[i], Data(ds).shell_ws[i], Data(ds).shell_h[i], 0,
         Data(ds).buffer_a, Data(ds).shell_save[i]);
    opeu_open_flash_toggle(ds, event);
  }
  return 0;
}

int opeu_open_shell_run_pair(uint8_t* ds, OpenEvent event) {
  int i, j;
  j = 3;
  for (i = 0; i < 3; i++, j++) {
    Call(ds, event, DRAW_FAR, Data(ds).shell_x[i], Data(ds).shell_y[i], Data(ds).shell_w[i], Data(ds).shell_h[i], 0x407,
         Data(ds).buffer_h, Data(ds).shell_sprite[i]);
    Call(ds, event, DRAW_FAR, Data(ds).shell_x[j], Data(ds).shell_y[j], Data(ds).shell_w[j], Data(ds).shell_h[j], 0x407,
         Data(ds).buffer_h, Data(ds).shell_sprite[j]);
    if (opeu_open_wait(ds, event, 1)) return 1;
    Call(ds, event, BLIT_FAR, Data(ds).shell_x[i], Data(ds).shell_y[i], Data(ds).shell_ws[i], Data(ds).shell_h[i], 0,
         Data(ds).buffer_a, Data(ds).shell_save[i]);
    Call(ds, event, BLIT_FAR, Data(ds).shell_x[j], Data(ds).shell_y[j], Data(ds).shell_ws[j], Data(ds).shell_h[j], 0,
         Data(ds).buffer_a, Data(ds).shell_save[j]);
    opeu_open_flash_toggle(ds, event);
  }
  return 0;
}

/* runs: pair salvos left, frame: frame, shot: 1 first, 2 pairs, 3 last. */
int opeu_open_gun_fire(uint8_t* ds, OpenEvent event) {
  int runs, frame, shot;
  for (frame = 0; frame < 6; frame++)
    Call(ds, event, BITMAP_SAVE, Data(ds).shell_x[frame], Data(ds).shell_y[frame], Data(ds).shell_ws[frame],
         Data(ds).shell_h[frame], Data(ds).buffer_a, Data(ds).shell_save[frame]);
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
    if (frame == 3 || frame == 12 || frame == 20 || frame == 23 || frame == 40 || frame == 49) Data(ds).flash_on ^= 1;
    if (shot == 1) {
      opeu_open_shell_run(ds, event, 0);
      shot = 2;
    } else if (shot == 2) {
      opeu_open_shell_run_pair(ds, event);
      if (--runs == 0) shot = 3;
    } else if (shot == 3) {
      opeu_open_shell_run(ds, event, 3);
      shot = 0;
    }
    if (!Data(ds).flash_on)
      Call(ds, event, PALETTE_SET, Palette(ds, 0x2f));
    else
      opeu_open_flash_toggle(ds, event);
    if (opeu_open_wait(ds, event, 1)) return 1;
  }
  if (opeu_open_wait(ds, event, 0x50)) return 1;
  if (opeu_open_wait_until(ds, event, 0xa8e2)) return 1;
  if (Call(ds, event, FADE_OUT)) return 1;
  return 0;
}

int opeu_open_scene_artillery(uint8_t* ds, OpenEvent event) {
  if (opeu_open_gun_setup(ds, event)) return 1;
  if (opeu_open_gun_pan(ds, event)) return 1;
  if (opeu_open_gun_fire(ds, event)) return 1;
  return 0;
}

int opeu_open_title(uint8_t* ds, OpenEvent event) {
  int i, start;
  opeu_open_clear_screen(ds, event);
  start = Data(ds).aborted ? 0x65 : 0x31;
  Call(ds, event, FADE_OUT_WAIT);
  opeu_open_fade_to_palette(ds, event, 0, 2);
  Call(ds, event, FADE_OUT);
  if (Data(ds).aborted) {
    opeu_open_fade_to_palette(ds, event, 0x6f, 2);
    opeu_open_delay(ds, event, 100);
  }
  PictureAt(ds, event, 0x37, 0x3c, 0x5ff59L);
  for (i = start; i <= 0x6f; i++) {
    opeu_open_fade_to_palette(ds, event, i, 2);
    if (Data(ds).aborted)
      opeu_open_delay(ds, event, 15);
    else
      opeu_open_delay(ds, event, 2);
  }
  opeu_open_delay(ds, event, 60);
  PictureAt(ds, event, 0x110, 0x148, 0x63a2cL);
  if (opeu_open_wait(ds, event, 0x103)) return 1;
  if (opeu_open_wait_until(ds, event, 0xdced)) return 1;
  MusicCommand4(ds, event, 0);
  while (Call(ds, event, MUSIC_PLAYING)) {
    if (Call(ds, event, INPUT_POLL)) return 1;
  }
  if (Call(ds, event, FADE_OUT)) return 1;
  opeu_open_clear_screen(ds, event);
  return 0;
}

void opeu_open_init(uint8_t* ds, OpenEvent event) {
  int ems, handle;
  if ((Data(ds).buffer_g = Call(ds, event, DOS_ALLOC, 0x501)) == 0) {
    Call(ds, event, CURSOR_PRINT, Near(ds, Data(ds).alloc_err1));
    Hang(ds, event);
  }
  if ((Data(ds).buffer_h = Call(ds, event, DOS_ALLOC, 0x6d5)) == 0) {
    Call(ds, event, CURSOR_PRINT, Near(ds, Data(ds).alloc_err2));
    Hang(ds, event);
  }
  if ((Data(ds).buffer_a = Call(ds, event, DOS_ALLOC, 0x1000)) == 0) {
    Call(ds, event, CURSOR_PRINT, Near(ds, Data(ds).alloc_err3));
    Hang(ds, event);
  }
  if ((Data(ds).buffer_b = Call(ds, event, DOS_ALLOC, 0x1000)) == 0) {
    Call(ds, event, CURSOR_PRINT, Near(ds, Data(ds).alloc_err4));
    Hang(ds, event);
  }
  if ((Data(ds).buffer_c = Call(ds, event, DOS_ALLOC, 0x1000)) == 0) {
    Call(ds, event, CURSOR_PRINT, Near(ds, Data(ds).alloc_err5));
    Hang(ds, event);
  }
  if ((Data(ds).buffer_d = Call(ds, event, DOS_ALLOC, 0x1000)) == 0) {
    Call(ds, event, CURSOR_PRINT, Near(ds, Data(ds).alloc_err6));
    Hang(ds, event);
  }
  if ((Data(ds).buffer_e = Call(ds, event, DOS_ALLOC, 0x1000)) == 0) {
    Call(ds, event, CURSOR_PRINT, Near(ds, Data(ds).alloc_err7));
    Hang(ds, event);
  }
  ems = Call(ds, event, EMS_DETECT);
  if ((Data(ds).buffer_f = Call(ds, event, DOS_ALLOC, 0x1000)) == 0 && (!ems || !Call(ds, event, EMS_ALLOC_SPRITE))) {
    Call(ds, event, CURSOR_PRINT, Near(ds, Data(ds).alloc_err9));
    Hang(ds, event);
  }
  handle = Call(ds, event, FILE_OR_DIE, Near(ds, Data(ds).name_mopengrp));
  Call(ds, event, DOS_READ_FAR, handle, Data(ds).buffer_h, 0, 0x6d40);
  Call(ds, event, DOS_CLOSE, handle);
  Call(ds, event, STUB_09B7, 1, 0);
  Call(ds, event, VGA_CLEAR);
  Call(ds, event, UI_TEXT, Near(ds, Data(ds).text_console));
  Call(ds, event, IDENTITY_PALETTE);
  Call(ds, event, SOUND_INITIALIZE);
  Call(ds, event, MOUSE_RESET);
}

void opeu_open_free_buffers(uint8_t* ds, OpenEvent event) {
  Call(ds, event, VGA_CLEAR);
  Call(ds, event, DOS_FREE, Data(ds).buffer_g);
  Call(ds, event, DOS_FREE, Data(ds).buffer_h);
  Call(ds, event, DOS_FREE, Data(ds).buffer_a);
  Call(ds, event, DOS_FREE, Data(ds).buffer_b);
  Call(ds, event, DOS_FREE, Data(ds).buffer_c);
  Call(ds, event, DOS_FREE, Data(ds).buffer_d);
  Call(ds, event, DOS_FREE, Data(ds).buffer_e);
  if (!Data(ds).sprite_mapped) Call(ds, event, DOS_FREE, Data(ds).buffer_f);
  Call(ds, event, MEMORY_RELEASE);
}

/* Music driver (int 66h): AH=1 AL=5, then AH=2 AL=20. */
void opeu_open_shutdown(uint8_t* ds, OpenEvent event) {
  Regs regs;
  Call(ds, event, FADE_OUT_WAIT);
  regs.h.ah = 1;
  regs.h.al = 5;
  Int86(ds, event, 0x66, regs);
  regs.h.ah = 2;
  regs.h.al = 20;
  Int86(ds, event, 0x66, regs);
  Call(ds, event, VGA_CLEAR);
  Call(ds, event, FADE_ALT);
  while (!Call(ds, event, DOS_EXISTS, Near(ds, Data(ds).name_scesel)));
  opeu_open_free_buffers(ds, event);
}

void opeu_open_files_open(uint8_t* ds, OpenEvent event) {
  Data(ds).picture_handle = Call(ds, event, FILE_OR_DIE, Near(ds, Data(ds).name_npk));
  Data(ds).logo_file = Call(ds, event, FILE_OR_DIE, Near(ds, Data(ds).name_fnt));
}

void opeu_open_files_close(uint8_t* ds, OpenEvent event) {
  Call(ds, event, DOS_CLOSE, Data(ds).picture_handle);
  Call(ds, event, DOS_CLOSE, Data(ds).logo_file);
}

void opeu_open_main(uint8_t* ds, OpenEvent event) {
  Call(ds, event, INSTALL_DOS_HANDLERS);
  opeu_open_init(ds, event);
  opeu_open_files_open(ds, event);
  for (;;) {
    Call(ds, event, TIMELINE_START, 20000);
    opeu_open_scene_koei(ds, event);
    if (opeu_open_scene_prologue(ds, event)) {
      Data(ds).aborted = 1;
      break;
    }
    if (opeu_open_scene_color_cycle(ds, event)) {
      Data(ds).aborted = 1;
      break;
    }
    if (opeu_open_scene_flicker(ds, event)) {
      Data(ds).aborted = 1;
      break;
    }
    if (opeu_open_scene_panorama(ds, event)) {
      Data(ds).aborted = 1;
      break;
    }
    if (opeu_open_scene_units(ds, event)) {
      Data(ds).aborted = 1;
      break;
    }
    if (opeu_open_scene_bombing(ds, event)) {
      Data(ds).aborted = 1;
      break;
    }
    if (opeu_open_scene_flak(ds, event)) {
      Data(ds).aborted = 1;
      break;
    }
    if (opeu_open_scene_artillery(ds, event)) {
      Data(ds).aborted = 1;
      break;
    }
    if (opeu_open_title(ds, event)) {
      Data(ds).aborted = 0;
      break;
    }
  }
  if (Data(ds).aborted) opeu_open_title(ds, event);
  opeu_open_files_close(ds, event);
  opeu_open_shutdown(ds, event);
  Call(ds, event, FM_NOOP);
}

}
