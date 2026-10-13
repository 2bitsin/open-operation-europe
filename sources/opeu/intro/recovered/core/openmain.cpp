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
 * free the far buffers (DS:1c22..1c32), read MOPENGRP.DAT, open and close OPENGRP.NPK/KOEILOGO.FNT, and wait
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

auto open_call(Open& open, OpenEvent event, OpenEventId id, uint16_t count, uint16_t a0, uint16_t a1, uint16_t a2, uint16_t a3, uint16_t a4, uint16_t a5, uint16_t a6) -> uint16_t {
  uint16_t args[7];
  args[0] = a0;
  args[1] = a1;
  args[2] = a2;
  args[3] = a3;
  args[4] = a4;
  args[5] = a5;
  args[6] = a6;
  return event(open, id, args, count);
}

/* One service call with its argument words. */
template <class... Words>
auto Call(Open& open, OpenEvent event, OpenEventId id, Words... words) -> uint16_t {
  uint16_t const padded[7]{ static_cast<uint16_t>(words)... };
  return open_call(open, event, id, sizeof...(Words), padded[0], padded[1], padded[2], padded[3], padded[4], padded[5], padded[6]);
}

/* The services whose arguments are converted on the way. */
auto PictureAt(Open& open, OpenEvent event, uint16_t x, uint16_t y, uint32_t position) -> uint16_t {
  return Call(open, event, PICTURE_AT, x, y, position, position >> 16);
}
auto DosSeek(Open& open, OpenEvent event, uint16_t handle, uint32_t position, uint16_t origin) -> uint16_t {
  return Call(open, event, DOS_SEEK, handle, position, position >> 16, origin);
}
auto MusicStart(Open& open, OpenEvent event, uint8_t track) -> uint16_t {
  return Call(open, event, MUSIC_START, track);
}
auto MusicCommand4(Open& open, OpenEvent event, uint8_t value) -> uint16_t {
  return Call(open, event, MUSIC_COMMAND4, value);
}
auto Int86(Open& open, OpenEvent event, uint16_t number, Regs const& in) -> uint16_t {
  return Call(open, event, INT86, number, in.h.al | in.h.ah << 8);
}

/* The near pointer of palette n. */
auto Palette(Open& open, unsigned n) -> uint16_t {
  return Near(open, Data(open).palettes[n]);
}

/* The allocation failures stop the program in an empty loop; the host
 * pumps its events instead (a loop without side effects is undefined in
 * C++), so the window still closes and a headless run still stops. */
auto Hang(Open& open, OpenEvent event) -> void {
  for (;;) Call(open, event, HANG);
}
}

auto opeu_open_wait(Open& open, OpenEvent event, uint16_t ticks) -> int;
auto opeu_open_wait_until(Open& open, OpenEvent event, uint16_t target) -> int;
void opeu_open_clear_screen(Open& open, OpenEvent event);
auto opeu_open_fade_to_palette(Open& open, OpenEvent event, uint8_t slot, int mode) -> int;
void opeu_open_load_koei_logo(Open& open, OpenEvent event);
void opeu_open_show_koei_logo(Open& open, OpenEvent event);
void opeu_open_show_logo_caption_a(Open& open, OpenEvent event);
void opeu_open_show_logo_caption_b(Open& open, OpenEvent event);
void opeu_open_scene_koei(Open& open, OpenEvent event);
auto opeu_open_prologue_pictures(Open& open, OpenEvent event) -> int;
auto opeu_open_scene_prologue(Open& open, OpenEvent event) -> int;
auto opeu_open_descend_animation(Open& open, OpenEvent event) -> int;
auto opeu_open_scene_color_cycle(Open& open, OpenEvent event) -> int;
auto opeu_open_scene_flicker(Open& open, OpenEvent event) -> int;
auto opeu_open_sprite_march(Open& open, OpenEvent event) -> int;
auto opeu_open_panorama_scroll(Open& open, OpenEvent event) -> int;
auto opeu_open_panorama_flashes(Open& open, OpenEvent event) -> int;
auto opeu_open_panorama_exit(Open& open, OpenEvent event) -> int;
auto opeu_open_scene_panorama(Open& open, OpenEvent event) -> int;
auto opeu_open_units_drive_in(Open& open, OpenEvent event) -> int;
auto opeu_open_units_animate(Open& open, OpenEvent event) -> int;
auto opeu_open_scene_units(Open& open, OpenEvent event) -> int;
auto opeu_open_bomber_approach(Open& open, OpenEvent event) -> int;
auto opeu_open_bomb_explosions(Open& open, OpenEvent event) -> int;
auto opeu_open_scene_bombing(Open& open, OpenEvent event) -> int;
auto opeu_open_flash_preload(Open& open, OpenEvent event) -> int;
auto opeu_open_bomber_flak(Open& open, OpenEvent event) -> int;
auto opeu_open_scene_flak(Open& open, OpenEvent event) -> int;
auto opeu_open_gun_setup(Open& open, OpenEvent event) -> int;
auto opeu_open_gun_pan(Open& open, OpenEvent event) -> int;
void opeu_open_flash_toggle(Open& open, OpenEvent event);
auto opeu_open_shell_run(Open& open, OpenEvent event, int first) -> int;
auto opeu_open_shell_run_pair(Open& open, OpenEvent event) -> int;
auto opeu_open_gun_fire(Open& open, OpenEvent event) -> int;
auto opeu_open_scene_artillery(Open& open, OpenEvent event) -> int;
auto opeu_open_title(Open& open, OpenEvent event) -> int;
void opeu_open_init(Open& open, OpenEvent event);
void opeu_open_free_buffers(Open& open, OpenEvent event);
void opeu_open_shutdown(Open& open, OpenEvent event);
void opeu_open_files_open(Open& open, OpenEvent event);
void opeu_open_files_close(Open& open, OpenEvent event);

void opeu_open_delay(Open& open, OpenEvent event, uint16_t ticks) {
  if (ticks != 0u) {
    Call(open, event, SFX_PLAY, ticks);
    while (Call(open, event, SFX_BUSY) != 0u);
  }
}

auto opeu_open_wait(Open& open, OpenEvent event, uint16_t ticks) -> int {
  uint16_t target  = 0;
  target           = Call(open, event, FM_BUSY);
  target          -= ticks;
  do {
    if (Call(open, event, INPUT_POLL) != 0u) return 1;
  } while (Call(open, event, FM_BUSY) > target);
  return 0;
}

auto opeu_open_wait_until(Open& open, OpenEvent event, uint16_t target) -> int {
  while (Call(open, event, FM_BUSY) > target) {
    if (Call(open, event, INPUT_POLL) != 0u) return 1;
  }
  return 0;
}

void opeu_open_clear_screen(Open& open, OpenEvent event) {
  Call(open, event, VGA_CLEAR);
}

auto opeu_open_fade_to_palette(Open& open, OpenEvent event, uint8_t slot, int mode) -> int {
  return Call(open, event, PALETTE_FADE, mode, Palette(open, slot));
}

void opeu_open_load_koei_logo(Open& open, OpenEvent event) {
  DosSeek(open, event, static_cast<uint16_t>(Data(open).logo_file), 0L, 0);
  Call(open, event, DOS_READ_FAR, Data(open).logo_file, Data(open).buffer_a, 0, 0xb60);
  DosSeek(open, event, static_cast<uint16_t>(Data(open).logo_file), 0x1860L, 0);
  Call(open, event, DOS_READ_FAR, Data(open).logo_file, Data(open).buffer_a, 0xb60, 0x270);
}

void opeu_open_show_koei_logo(Open& open, OpenEvent event) {
  opeu_open_clear_screen(open, event);
  opeu_open_fade_to_palette(open, event, 2, -1);
  Call(open, event, DRAW_FAR, 0x1b, 0x60, 0x1a, 0x70, 2, Data(open).buffer_a, 0);
  opeu_open_delay(open, event, 60);
  Call(open, event, DRAW_FAR, 0x1b, 0xe0, 0x1a, 0x18, 7, Data(open).buffer_a, 0xb60);
  opeu_open_delay(open, event, 60);
  opeu_open_wait_until(open, event, 0xe98a);
  Call(open, event, FADE_OUT_WAIT);
}

void opeu_open_show_logo_caption_a(Open& open, OpenEvent event) {
  opeu_open_clear_screen(open, event);
  opeu_open_fade_to_palette(open, event, 2, -1);
  PictureAt(open, event, 0x80, 0xbc, 0L);
  opeu_open_delay(open, event, 0x78);
  opeu_open_wait_until(open, event, 0xe868);
  Call(open, event, FADE_OUT_WAIT);
}

void opeu_open_show_logo_caption_b(Open& open, OpenEvent event) {
  opeu_open_clear_screen(open, event);
  opeu_open_fade_to_palette(open, event, 2, -1);
  PictureAt(open, event, 0xf2, 0x76, 0x7f0L);
  PictureAt(open, event, 0x80, 0xba, 0x42dL);
  opeu_open_delay(open, event, 0x78);
  opeu_open_wait_until(open, event, 0xe50c);
  Call(open, event, FADE_OUT_WAIT);
}

void opeu_open_scene_koei(Open& open, OpenEvent event) {
  opeu_open_load_koei_logo(open, event);
  opeu_open_show_koei_logo(open, event);
  opeu_open_show_logo_caption_a(open, event);
  opeu_open_show_logo_caption_b(open, event);
}

auto opeu_open_prologue_pictures(Open& open, OpenEvent event) -> int {
  opeu_open_clear_screen(open, event);
  PictureAt(open, event, 0, 0, 0x2955fL);
  Call(open, event, BITMAP_SAVE, 0, 0, 0x50, 0x8c, Data(open).buffer_e, 0);
  Call(open, event, BITMAP_SAVE, 0, 0x8c, 0x50, 0x8c, Data(open).buffer_f, 0);
  opeu_open_clear_screen(open, event);
  PictureAt(open, event, 0x58, 0, 0xa81L);
  if (opeu_open_fade_to_palette(open, event, 3, 2) != 0) return 1;
  Call(open, event, BITMAP_SAVE, 10, 0, 0x3c, 0xc8, Data(open).buffer_b, 0);
  Call(open, event, BITMAP_SAVE, 10, 0xc8, 0x3c, 0xc8, Data(open).buffer_c, 0);
  if (opeu_open_wait(open, event, 0x105) != 0) return 1;
  if (opeu_open_wait_until(open, event, 0xe25d) != 0) return 1;
  if (Call(open, event, FADE_OUT) != 0u) return 1;
  return 0;
}

auto opeu_open_scene_prologue(Open& open, OpenEvent event) -> int {
  if (opeu_open_wait_until(open, event, 0xe4bf) != 0) return 1;
  MusicStart(open, event, 0);
  MusicCommand4(open, event, 1);
  if (opeu_open_prologue_pictures(open, event) != 0) return 1;
  opeu_open_clear_screen(open, event);
  PictureAt(open, event, 0x90, 0x3c, 0x93f7L);
  if (opeu_open_fade_to_palette(open, event, 4, 2) != 0) return 1;
  Call(open, event, BITMAP_SAVE, 0x12, 0x3c, 0x30, 0x154, Data(open).buffer_d, 0);
  if (opeu_open_wait(open, event, 0xdc) != 0) return 1;
  if (opeu_open_wait_until(open, event, 0xdfa9) != 0) return 1;
  if (Call(open, event, FADE_OUT) != 0u) return 1;
  return 0;
}

auto opeu_open_descend_animation(Open& open, OpenEvent event) -> int {
  uint16_t h     = 0;
  int      i     = 0;
  uint16_t off_a = 0;
  uint16_t off_b = 0;
  opeu_open_clear_screen(open, event);
  if (opeu_open_fade_to_palette(open, event, 30, 2) != 0) return 1;
  h     = 0;
  off_a = 0x2580;
  off_b = 0x4a40;
  for (i = 0; i < 25; i++) {
    Call(open, event, BLIT_FAR, 0x22, 0, 12, h += 2, 0, Data(open).buffer_a, (off_a -= 0x60));
    if (opeu_open_wait(open, event, 6) != 0) return 1;
    if (i > 21) Call(open, event, PALETTE_STEP, Palette(open, 0));
    Call(open, event, BLIT_FAR, 0x22, 0, 12, h += 2, 0, Data(open).buffer_a, (off_a -= 0x60));
    if (opeu_open_wait(open, event, 6) != 0) return 1;
    if (i > 21) Call(open, event, PALETTE_STEP, Palette(open, 0));
    off_a -= 0xc0;
    Call(open, event, BLIT_FAR, 0x22, 0, 12, h += 2, 0, Data(open).buffer_a, (off_b -= 0x60));
    if (opeu_open_wait(open, event, 6) != 0) return 1;
    if (i > 21) Call(open, event, PALETTE_STEP, Palette(open, 0));
    Call(open, event, BLIT_FAR, 0x22, 0, 12, h += 2, 0, Data(open).buffer_a, (off_b -= 0x60));
    if (opeu_open_wait(open, event, 6) != 0) return 1;
    if (i > 21) Call(open, event, PALETTE_STEP, Palette(open, 0));
    off_b -= 0xc0;
  }
  if (Call(open, event, FADE_OUT) != 0u) return 1;
  return 0;
}

auto opeu_open_scene_color_cycle(Open& open, OpenEvent event) -> int {
  uint8_t pal = 0;
  int     j   = 0;
  opeu_open_clear_screen(open, event);
  PictureAt(open, event, 0x58, 0, 0x121a0L);
  if (opeu_open_fade_to_palette(open, event, 5, 2) != 0) return 1;
  if (opeu_open_wait(open, event, 0x103) != 0) return 1;
  if (opeu_open_wait_until(open, event, 0xdced) != 0) return 1;
  Call(open, event, PALETTE_SET, Palette(open, 6));
  if (opeu_open_wait(open, event, 5) != 0) return 1;
  Call(open, event, PALETTE_SET, Palette(open, 7));
  if (opeu_open_wait(open, event, 5) != 0) return 1;
  Call(open, event, PALETTE_SET, Palette(open, 8));
  if (opeu_open_wait(open, event, 5) != 0) return 1;
  Call(open, event, PALETTE_SET, Palette(open, 9));
  if (opeu_open_wait(open, event, 5) != 0) return 1;
  Call(open, event, PALETTE_SET, Palette(open, 5));
  if (opeu_open_wait(open, event, 0x14) != 0) return 1;
  if (Call(open, event, FADE_OUT) != 0u) return 1;
  opeu_open_clear_screen(open, event);
  PictureAt(open, event, 0, 0, 0x28343L);
  PictureAt(open, event, 0x60, 0, 0x28c1bL);
  Call(open, event, BITMAP_SAVE, 0, 0, 12, 0xc8, Data(open).buffer_a, 0);
  Call(open, event, BITMAP_SAVE, 12, 0, 12, 0xc8, Data(open).buffer_a, 0x2580);
  opeu_open_clear_screen(open, event);
  PictureAt(open, event, 0, 0, 0x27b7dL);
  for (pal = 11, j = 0; j < 19; j++, pal++) {
    Call(open, event, PALETTE_SET, Palette(open, pal));
    if (opeu_open_wait(open, event, 5) != 0) return 1;
  }
  if (opeu_open_fade_to_palette(open, event, 0, 1) != 0) return 1;
  if (opeu_open_descend_animation(open, event) != 0) return 1;
  return 0;
}

auto opeu_open_scene_flicker(Open& open, OpenEvent event) -> int {
  int i   = 0;
  int off = 0;
  opeu_open_clear_screen(open, event);
  Call(open, event, BLIT_FAR, 0, 0x3c, 0x50, 0x8c, 0, Data(open).buffer_e, 0);
  Call(open, event, BLIT_FAR, 0, 0xc8, 0x50, 0x8c, 0, Data(open).buffer_f, 0);
  if (opeu_open_fade_to_palette(open, event, 31, 2) != 0) return 1;
  for (i = 0; i < 2; i++) {
    Call(open, event, PALETTE_SET, Palette(open, 32));
    if (opeu_open_wait(open, event, 6) != 0) return 1;
    Call(open, event, PALETTE_SET, Palette(open, 31));
    if (opeu_open_wait(open, event, 6) != 0) return 1;
  }
  for (i = 0; i < 10; i++) {
    Call(open, event, PALETTE_SET, Palette(open, 32));
    if (opeu_open_wait(open, event, 5) != 0) return 1;
    Call(open, event, PALETTE_SET, Palette(open, 31));
    if (opeu_open_wait(open, event, 8) != 0) return 1;
  }
  if (opeu_open_wait(open, event, 0x78) != 0) return 1;
  if (Call(open, event, FADE_OUT) != 0u) return 1;
  opeu_open_clear_screen(open, event);
  PictureAt(open, event, 0, 0, 0x399b9L);
  Call(open, event, BITMAP_SAVE, 0, 0, 0x12, 0x78, Data(open).buffer_a, 0);
  Call(open, event, BITMAP_SAVE, 0x12, 0, 0x12, 0x78, Data(open).buffer_a, 0x21c0);
  Call(open, event, BITMAP_SAVE, 0x24, 0, 0x12, 0x78, Data(open).buffer_a, 0x4380);
  Call(open, event, BITMAP_SAVE, 0, 0x78, 0x12, 0x78, Data(open).buffer_a, 0x6540);
  Call(open, event, BITMAP_SAVE, 0x12, 0x78, 0x12, 0x78, Data(open).buffer_a, 0x8700);
  Call(open, event, BITMAP_SAVE, 0x24, 0x78, 0x12, 0x78, Data(open).buffer_a, 0xa8c0);
  opeu_open_clear_screen(open, event);
  Call(open, event, BLIT_FAR, 0x12, 0x3c, 0x30, 0x154, 0, Data(open).buffer_d, 0);
  if (opeu_open_wait(open, event, 0x3c) != 0) return 1;
  PictureAt(open, event, 0xc, 0x3c, 0x37110L);
  Call(open, event, BITMAP_SAVE, 0, 0x3c, 0x26, 0x120, Data(open).buffer_e, 0);
  opeu_open_clear_screen(open, event);
  Call(open, event, BLIT_FAR, 0x12, 0x3c, 0x30, 0x154, 0, Data(open).buffer_d, 0);
  if (opeu_open_fade_to_palette(open, event, 4, 2) != 0) return 1;
  if (opeu_open_wait_until(open, event, 0xd56a) != 0) return 1;
  Call(open, event, DISSOLVE, 0, 0x3c, Data(open).buffer_e, 0, 0x0001);
  Call(open, event, DISSOLVE, 0, 0x3c, Data(open).buffer_e, 0, 0x0101);
  Call(open, event, DISSOLVE, 0, 0x3c, Data(open).buffer_e, 0, 0x1111);
  Call(open, event, DISSOLVE, 0, 0x3c, Data(open).buffer_e, 0, 0x5555);
  Call(open, event, DISSOLVE, 0, 0x3c, Data(open).buffer_e, 0, 0x7777);
  Call(open, event, DISSOLVE, 0, 0x3c, Data(open).buffer_e, 0, 0x7f7f);
  Call(open, event, DISSOLVE, 0, 0x3c, Data(open).buffer_e, 0, 0xffff);
  if (opeu_open_wait(open, event, 0xc8) != 0) return 1;
  if (opeu_open_wait_until(open, event, 0xd3a2) != 0) return 1;
  if (Call(open, event, FADE_OUT) != 0u) return 1;
  opeu_open_clear_screen(open, event);
  PictureAt(open, event, 0, 0, 0x3e98bL);
  for (off = 0, i = 78; i >= 0; i--) {
    Call(open, event, BITMAP_SAVE, i, 0, 1, 0xb0, Data(open).buffer_d, static_cast<uint16_t>(off));
    off += 0x2c0;
  }
  return 0;
}

auto opeu_open_sprite_march(Open& open, OpenEvent event) -> int {
  uint16_t d     = 0;
  int      pass  = 0;
  int      j     = 0;
  uint16_t off_a = 0;
  uint16_t off_b = 0;
  opeu_open_clear_screen(open, event);
  Call(open, event, BLIT_FAR, 10, 0, 0x3c, 0xc8, 0, Data(open).buffer_b, 0);
  Call(open, event, BLIT_FAR, 10, 0xc8, 0x3c, 0xc8, 0, Data(open).buffer_c, 0);
  if (opeu_open_fade_to_palette(open, event, 3, 2) != 0) return 1;
  if (opeu_open_wait_until(open, event, 0xd2ed) != 0) return 1;
  for (d = 0x40, pass = 0;; d >>= 1, d++, pass++) {
    for (off_a = 0, off_b = 0x6540, j = 0; j < 3; j++) {
      Call(open, event, BLIT_FAR, 10, 0x118, 0x12, 0x78, 0, Data(open).buffer_a, off_a);
      Call(open, event, BLIT_FAR, 0x34, 0x118, 0x12, 0x78, 0, Data(open).buffer_a, off_b);
      off_a += 0x21c0;
      off_b += 0x21c0;
      if (opeu_open_wait(open, event, d) != 0) return 1;
    }
    if (pass > 40 && (Call(open, event, PALETTE_STEP, Palette(open, 0)) == 0u)) {
      if (Call(open, event, FADE_OUT) != 0u) return 1;
      break;
    }
  }
  return 0;
}

auto opeu_open_panorama_scroll(Open& open, OpenEvent event) -> int {
  int      i   = 0;
  uint16_t off = 0;
  opeu_open_clear_screen(open, event);
  PictureAt(open, event, 0, 0, 0x40bd5L);
  Call(open, event, BITMAP_SAVE, 0, 0, 0x20, 0xb0, Data(open).buffer_a, 0);
  if (opeu_open_wait(open, event, 1) != 0) return 1;
  PictureAt(open, event, 0, 0, 0x421d5L);
  Call(open, event, BITMAP_SAVE, 0, 0, 0x20, 0xb0, Data(open).buffer_b, 0);
  if (opeu_open_wait(open, event, 1) != 0) return 1;
  PictureAt(open, event, 0, 0, 0x43202L);
  Call(open, event, BITMAP_SAVE, 0, 0, 0x20, 0xb0, Data(open).buffer_c, 0);
  opeu_open_clear_screen(open, event);
  if (opeu_open_fade_to_palette(open, event, 33, 0) != 0) return 1;
  for (off = 0, i = 0; i < 0x4f; i++) {
    Call(open, event, SCREEN_COPY, 0x19, 0x70, 0x1d, 0xb0, 0x1a, 0x70);
    Call(open, event, BLIT_FAR, 0x19, 0x70, 1, 0xb0, 0, Data(open).buffer_d, off);
    if (opeu_open_wait(open, event, 5) != 0) return 1;
    off += 0x2c0;
  }
  Call(open, event, BITMAP_SAVE, 0x19, 0x70, 0x20, 0xb0, Data(open).buffer_d, 0);
  if (opeu_open_wait_until(open, event, 0xcc15) != 0) return 1;
  return 0;
}

auto opeu_open_panorama_flashes(Open& open, OpenEvent event) -> int {
  int i = 0;
  for (i = 0; i < 2; i++) {
    Call(open, event, PALETTE_SET, Palette(open, 34));
    Call(open, event, PALETTE_SET, Palette(open, 35));
    if (opeu_open_wait(open, event, 2) != 0) return 1;
    Call(open, event, BLIT_FAR, 0x18, 0x70, 0x20, 0xb0, 0, Data(open).buffer_a, 0);
    Call(open, event, PALETTE_SET, Palette(open, 33));
    if (opeu_open_wait(open, event, 10) != 0) return 1;
    Call(open, event, BLIT_FAR, 0x18, 0x70, 0x20, 0xb0, 0, Data(open).buffer_b, 0);
    if (opeu_open_wait(open, event, 10) != 0) return 1;
    Call(open, event, BLIT_FAR, 0x18, 0x70, 0x20, 0xb0, 0, Data(open).buffer_c, 0);
    if (opeu_open_wait(open, event, 10) != 0) return 1;
    Call(open, event, BLIT_FAR, 0x18, 0x70, 0x20, 0xb0, 0, Data(open).buffer_d, 0);
    if (i == 0 && (opeu_open_wait(open, event, 100) != 0)) return 1;
  }
  if (opeu_open_wait(open, event, 0x1e) != 0) return 1;
  if (opeu_open_wait_until(open, event, 0xc9f8) != 0) return 1;
  return 0;
}

auto opeu_open_panorama_exit(Open& open, OpenEvent event) -> int {
  int i = 0;
  Call(open, event, UI_BOX, 0x38, 0x70, 1, 0xb0, 0);
  for (i = 0; i < 15; i++) {
    Call(open, event, SCREEN_COPY, 0x17, 0x70, 0x1f, 0xb0, 0x18, 0x70);
    if (opeu_open_wait(open, event, 5) != 0) return 1;
  }
  if (opeu_open_wait_until(open, event, 0xc99e) != 0) return 1;
  if (Call(open, event, FADE_OUT) != 0u) return 1;
  PictureAt(open, event, 0, 0, 0x43fd2L);
  Call(open, event, BITMAP_SAVE, 0, 0, 0x14, 0xc0, Data(open).buffer_a, 0);
  Call(open, event, BITMAP_SAVE, 0x14, 0, 0x14, 0xc0, Data(open).buffer_a, 0x3c00);
  Call(open, event, BITMAP_SAVE, 0x28, 0, 0x14, 0xc0, Data(open).buffer_a, 0x7800);
  Call(open, event, BITMAP_SAVE, 0x3c, 0, 0x14, 0xc0, Data(open).buffer_a, 0xb400);
  Call(open, event, BITMAP_SAVE, 0, 0xc0, 0x14, 0xc0, Data(open).buffer_b, 0);
  Call(open, event, BITMAP_SAVE, 0x14, 0xc0, 0x14, 0xc0, Data(open).buffer_b, 0x3c00);
  Call(open, event, BITMAP_SAVE, 0x28, 0xc0, 0x14, 0xc0, Data(open).buffer_b, 0x7800);
  Call(open, event, BITMAP_SAVE, 0x3c, 0xc0, 0x14, 0xc0, Data(open).buffer_b, 0xb400);
  return 0;
}

auto opeu_open_scene_panorama(Open& open, OpenEvent event) -> int {
  if (opeu_open_sprite_march(open, event) != 0) return 1;
  if (opeu_open_panorama_scroll(open, event) != 0) return 1;
  if (opeu_open_panorama_flashes(open, event) != 0) return 1;
  if (opeu_open_panorama_exit(open, event) != 0) return 1;
  return 0;
}
/* x: strip column, off_a/off_b: strip offsets in buffer C. */
auto opeu_open_units_drive_in(Open& open, OpenEvent event) -> int {
  int      x     = 0;
  uint16_t off_a = 0;
  uint16_t off_b = 0;
  opeu_open_clear_screen(open, event);
  Call(open, event, BLIT_FAR, 2, 0, 0x14, 0x60, 0, Data(open).buffer_a, 0);
  off_a = 0;
  for (x = 0x15; x > 0; x -= 2) {
    Call(open, event, BITMAP_SAVE, x, 0, 2, 0x60, Data(open).buffer_c, off_a);
    off_a += 0x300;
  }
  for (x = 0x14; x >= 0; x -= 2) {
    Call(open, event, BITMAP_SAVE, x, 0, 2, 0x60, Data(open).buffer_c, off_a);
    off_a += 0x300;
  }
  opeu_open_clear_screen(open, event);
  Call(open, event, PALETTE_SET, Palette(open, 36));
  x = off_b = off_a = 0;
  for (; x < 31; x++) {
    Call(open, event, SCREEN_COPY, 0, 0xc4, 0x3c, 0x60, 2, 0xc4);
    if (x < 11) {
      Call(open, event, BLIT_FAR, 0, 0xc4, 2, 0x60, 0, Data(open).buffer_c, off_a);
      off_a += 0x300;
    }
    if (x > 12 && x < 24) {
      Call(open, event, BLIT_FAR, 0, 0xc4, 2, 0x60, 0, Data(open).buffer_c, off_b);
      off_b += 0x300;
    }
    if (opeu_open_wait(open, event, 6) != 0) return 1;
  }
  if (opeu_open_wait(open, event, 10) != 0) return 1;
  return 0;
}

/* i: frame count, j/k: frame of the right/left vehicle. */
auto opeu_open_units_animate(Open& open, OpenEvent event) -> int {
  int i = 0;
  int j = 0;
  int k = 0;
  i = k = j = 0;
  for (; i < 40; i++) {
    if (i == 0) {
      Call(open, event, PALETTE_SET, Palette(open, 34));
      Call(open, event, PALETTE_SET, Palette(open, 35));
      if (opeu_open_wait(open, event, 2) != 0) return 1;
      Call(open, event, PALETTE_SET, Palette(open, 36));
    }
    Call(open, event, UI_BOX, 0x2a, 0x64, 0x14, 0x80, 0);
    Call(open, event, BLIT_FAR, 0x2a, Data(open).frame_y[j], 0x14, Data(open).frame_h[j], 0, j < 5 ? Data(open).buffer_a : Data(open).buffer_b, Data(open).frame_off[j]);
    j++;
    if (j == 11) j = 8;
    Call(open, event, UI_BOX, 0x10, 0x64, 0x14, 0x80, 0);
    Call(open, event, BLIT_FAR, 0x10, Data(open).frame_y[k], 0x14, Data(open).frame_h[k], 0, k < 5 ? Data(open).buffer_a : Data(open).buffer_b, Data(open).frame_off[k]);
    if (i > 2) {
      k++;
      if (k == 11) k = 8;
    }
    if (opeu_open_wait(open, event, 10) != 0) return 1;
    if (i > 30) Call(open, event, PALETTE_STEP, Palette(open, 0));
  }
  if (Call(open, event, FADE_OUT) != 0u) return 1;
  if (opeu_open_wait_until(open, event, 0xc5e2) != 0) return 1;
  return 0;
}

auto opeu_open_scene_units(Open& open, OpenEvent event) -> int {
  if (opeu_open_units_drive_in(open, event) != 0) return 1;
  if (opeu_open_units_animate(open, event) != 0) return 1;
  return 0;
}

/* a: composite toggle, b: frame deadline, c: bob index, d: frame,
 * e: y, f: faded in, g: rotating part offset, h: next y. */
auto opeu_open_bomber_approach(Open& open, OpenEvent event) -> int {
  uint16_t a = 0;
  uint16_t b = 0;
  int      c = 0;
  int      d = 0;
  uint16_t e = 0;
  int      f = 0;
  uint16_t g = 0;
  uint16_t h = 0;
  opeu_open_clear_screen(open, event);
  PictureAt(open, event, 0, 0, 0x46848L);
  Call(open, event, BITMAP_SAVE, 0, 0, 0x50, 0xa0, Data(open).buffer_c, 0);
  opeu_open_clear_screen(open, event);
  PictureAt(open, event, 0, 0, 0x487fbL);
  Call(open, event, BITMAP_SAVE, 0, 0, 8, 0x40, Data(open).buffer_a, 0);
  Call(open, event, BITMAP_SAVE, 8, 0, 8, 0x40, Data(open).buffer_a, 0x800);
  Call(open, event, BITMAP_SAVE, 0x10, 0, 8, 0x40, Data(open).buffer_a, 0x1000);
  Call(open, event, BITMAP_SAVE, 0x18, 0, 8, 0x40, Data(open).buffer_a, 0x1800);
  Call(open, event, BITMAP_SAVE, 0x20, 0, 8, 0x40, Data(open).buffer_a, 0x2000);
  PictureAt(open, event, 0, 0, 0x48ce9L);
  Call(open, event, BITMAP_SAVE, 0, 0, 0x32, 0x140, Data(open).buffer_b, 0);
  PictureAt(open, event, 0, 0, 0x4d847L);
  Call(open, event, BITMAP_SAVE, 0, 0, 0x1a, 0xb0, Data(open).buffer_e, 0);
  Call(open, event, BITMAP_SAVE, 0x1a, 0, 0x1a, 0xb0, Data(open).buffer_e, 0x4780);
  Call(open, event, BITMAP_SAVE, 0x34, 0, 0x1a, 0xb0, Data(open).buffer_e, 0x8f00);
  Call(open, event, BITMAP_SAVE, 0, 0xb0, 0x1a, 0xb0, Data(open).buffer_f, 0);
  Call(open, event, BITMAP_SAVE, 0x1a, 0xb0, 0x1a, 0xb0, Data(open).buffer_f, 0x4780);
  Call(open, event, BITMAP_SAVE, 0x34, 0xb0, 0x1a, 0xb0, Data(open).buffer_f, 0x8f00);
  opeu_open_clear_screen(open, event);
  PictureAt(open, event, 0, 0, 0x47428L);
  Call(open, event, BITMAP_SAVE, 0, 0, 0x10, 0x90, Data(open).buffer_a, 0x2800);
  Call(open, event, BITMAP_SAVE, 0x10, 0, 0x10, 0x90, Data(open).buffer_a, 0x7000);
  Call(open, event, BITMAP_SAVE, 0x20, 0, 0x10, 0x90, Data(open).buffer_a, 0x4c00);
  Call(open, event, BITMAP_SAVE, 0x30, 0, 0x10, 0x90, Data(open).buffer_a, 0x9400);
  opeu_open_clear_screen(open, event);
  Call(open, event, BLIT_FAR, 0, 0x50, 0x50, 0xa0, 0, Data(open).buffer_c, 0);
  Call(open, event, BLIT_FAR, 0x13, 0x50, 0x10, 0x90, 0, Data(open).buffer_a, 0x2800);
  Call(open, event, BLIT_FAR, 0x32, 0x50, 0x10, 0x90, 0, Data(open).buffer_a, 0x4c00);
  Call(open, event, BLIT_FAR, 0, 0xf0, 0x50, 0xa0, 0, Data(open).buffer_c, 0);
  Call(open, event, BLIT_FAR, 0x13, 0xf0, 0x10, 0x90, 0, Data(open).buffer_a, 0x7000);
  Call(open, event, BLIT_FAR, 0x32, 0xf0, 0x10, 0x90, 0, Data(open).buffer_a, 0x9400);
  Call(open, event, BITMAP_SAVE, 0, 0x50, 0x50, 0xa0, Data(open).buffer_c, 0);
  Call(open, event, BITMAP_SAVE, 0, 0xf0, 0x50, 0xa0, Data(open).buffer_d, 0);
  opeu_open_clear_screen(open, event);
  Call(open, event, BLIT_FAR, 0, 0x50, 0x50, 0xa0, 0, Data(open).buffer_d, 0);
  h = e = Data(open).bob_a[0];
  d = f = c = a = g = 0;
  for (; d < 0x3a; d++) {
    b  = Call(open, event, FM_BUSY);
    b -= 12;
    e  = Data(open).bob_a[c];
    Call(open, event, BLIT_FAR, 0, e, 0x50, 0xa0, 0, (a != 0u) ? Data(open).buffer_d : Data(open).buffer_c, 0);
    Call(open, event, BLIT_FAR, 0x23, e + 0x60, 8, 0x40, 0, Data(open).buffer_a, g);
    a ^= 1;
    c++;
    if (c > 19) c = 0;
    h = Data(open).bob_a[c];
    if (h < e) Call(open, event, UI_BOX, 0, h + 0xa0, 0x50, e - h, 0);
    g += 0x800;
    if (g >= 0x2800) g = 0;
    if ((f == 0) && (Call(open, event, PALETTE_STEP, Palette(open, 0x25)) == 0u)) f = 1;

    while (Call(open, event, FM_BUSY) > b) {
      if (Call(open, event, INPUT_POLL) != 0u) return 1;
    }
  }
  if (Call(open, event, FADE_OUT) != 0u) return 1;
  return 0;
}

/* k: explosion, j: frame, n: palette step, pos: frame offset. */
// The opening's recovered animation script; it goes
// when the opening becomes a scene (M6).
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
auto opeu_open_bomb_explosions(Open& open, OpenEvent event) -> int {
  int      k   = 0;
  int      j   = 0;
  int      n   = 0;
  uint16_t pos = 0;
  opeu_open_clear_screen(open, event);
  Call(open, event, PALETTE_SET, Palette(open, 0x26));
  Call(open, event, BLIT_FAR, 0xf, 0x28, 0x32, 0x140, 0, Data(open).buffer_b, 0);
  if (opeu_open_wait_until(open, event, 0xbd4b) != 0) return 1;
  if (opeu_open_wait(open, event, 50) != 0) return 1;
  for (k = 0; k < 3; k++) {
    Call(open, event, BITMAP_SAVE, Data(open).blast_x[k], Data(open).blast_y[k], 0x1a, 0xb0, Data(open).buffer_a, 0xb800);
    for (pos = 0, j = 0; j < 3; j++, pos += 0x4780) {
      Call(open, event, FAR_COPY, Data(open).buffer_a, 0xb800, Data(open).buffer_b, pos, 0x4780);
      Call(open, event, BUF_AND_MASK, Data(open).buffer_b, pos, 0x1a, 0xb0, Data(open).buffer_h, Data(open).blast_masks[j]);
      Call(open, event, BUF_OR, Data(open).buffer_b, pos, 0x1a, 0xb0, Data(open).buffer_e, Data(open).blast_sprites[j]);
    }
    for (pos = 0, j = 3; j < 6; j++, pos += 0x4780) {
      Call(open, event, FAR_COPY, Data(open).buffer_a, 0xb800, Data(open).buffer_c, pos, 0x4780);
      Call(open, event, BUF_AND_MASK, Data(open).buffer_c, pos, 0x1a, 0xb0, Data(open).buffer_h, Data(open).blast_masks[j]);
      Call(open, event, BUF_OR, Data(open).buffer_c, pos, 0x1a, 0xb0, Data(open).buffer_f, Data(open).blast_sprites[j]);
    }
    for (pos = 0, j = 0; j < 6; j++) {
      Call(open, event, BLIT_FAR, Data(open).blast_x[k], Data(open).blast_y[k], 0x1a, 0xb0, 4, j < 3 ? Data(open).buffer_b : Data(open).buffer_c, pos);
      if (pos == 0x8f00)
        pos = 0;
      else
        pos += 0x4780;
      for (n = 0; n < Data(open).blast_steps[j]; n++) Call(open, event, PALETTE_STEP, Palette(open, Data(open).blast_slots[k]));
      if (opeu_open_wait(open, event, 2) != 0) return 1;
    }
    Call(open, event, BLIT_FAR, Data(open).blast_x[k], Data(open).blast_y[k], 0x1a, 0xb0, 0, Data(open).buffer_a, 0xb800);
    if (opeu_open_fade_to_palette(open, event, 0x26, 1) != 0) return 1;
    Call(open, event, BLIT_FAR, Data(open).blast_x[k], Data(open).blast_y[k], 0x1a, 0xb0, 0, Data(open).buffer_a, 0xb800);
    if (opeu_open_wait(open, event, 10) != 0) return 1;
  }
  if (opeu_open_wait(open, event, 90) != 0) return 1;
  if (opeu_open_wait_until(open, event, 0xbada) != 0) return 1;
  if (Call(open, event, FADE_OUT) != 0u) return 1;
  return 0;
}

auto opeu_open_scene_bombing(Open& open, OpenEvent event) -> int {
  if (opeu_open_bomber_approach(open, event) != 0) return 1;
  if (opeu_open_bomb_explosions(open, event) != 0) return 1;
  return 0;
}

auto opeu_open_flash_preload(Open& open, OpenEvent event) -> int {
  int i = 0;
  opeu_open_clear_screen(open, event);
  PictureAt(open, event, 0x4c, 0xc, 0x54d6dL);
  if (opeu_open_fade_to_palette(open, event, 0x2a, 2) != 0) return 1;
  if (opeu_open_wait(open, event, 100) != 0) return 1;
  if (opeu_open_wait_until(open, event, 0xb8d3) != 0) return 1;
  for (i = 0; i < 3; i++) {
    Call(open, event, PALETTE_SET, Palette(open, Data(open).flash_slots[i]));
    while (Call(open, event, PALETTE_STEP, Palette(open, 0x2a)) != 0u);
    if (opeu_open_wait(open, event, 60) != 0) return 1;
  }
  if (opeu_open_wait(open, event, 30) != 0) return 1;
  if (opeu_open_wait_until(open, event, 0xb6f8) != 0) return 1;
  if (Call(open, event, FADE_OUT) != 0u) return 1;
  opeu_open_clear_screen(open, event);
  PictureAt(open, event, 0, 0xa, 0x5b770L);
  Call(open, event, BITMAP_SAVE, 0, 0, 6, 0x3c, Data(open).buffer_b, 0);
  Call(open, event, BITMAP_SAVE, 6, 0, 6, 0x3c, Data(open).buffer_b, 0x5a0);
  Call(open, event, BITMAP_SAVE, 0xc, 0, 6, 0x3c, Data(open).buffer_b, 0xb40);
  Call(open, event, BITMAP_SAVE, 0x12, 0, 6, 0x3c, Data(open).buffer_b, 0x10e0);
  opeu_open_clear_screen(open, event);
  PictureAt(open, event, 0, 0, 0x5b9e3L);
  Call(open, event, BITMAP_SAVE, 0, 0, 0x20, 0xe0, Data(open).buffer_e, 0);
  PictureAt(open, event, 0, 0, 0x5d486L);
  Call(open, event, BITMAP_SAVE, 0, 0, 0x20, 0xe0, Data(open).buffer_e, 0x7000);
  if (opeu_open_wait(open, event, 15) != 0) return 1;
  if (opeu_open_wait_until(open, event, 0xb6f8) != 0) return 1;
  if (Call(open, event, FADE_OUT) != 0u) return 1;
  return 0;
}

/* a: composite toggle, b: frame % 10, d: deadline, e: bob index,
 * f: frame, g: y, h: rotating part offset, i: next y, j: burst offset. */
auto opeu_open_bomber_flak(Open& open, OpenEvent event) -> int {
  uint16_t a = 0;
  int      b = 0;
  uint16_t d = 0;
  int      e = 0;
  int      f = 0;
  uint16_t g = 0;
  uint16_t h = 0;
  uint16_t i = 0;
  uint16_t j = 0;
  opeu_open_clear_screen(open, event);
  Call(open, event, BLIT_FAR, 0, 0x50, 0x50, 0xa0, 0, Data(open).buffer_d, 0);
  Call(open, event, BLIT_FAR, 0x13, 0x50, 0x10, 0x90, 0, Data(open).buffer_a, 0x2800);
  Call(open, event, BLIT_FAR, 0x32, 0x50, 0x10, 0x90, 0, Data(open).buffer_a, 0x4c00);
  Call(open, event, BITMAP_SAVE, 0, 0x50, 0x50, 0xa0, Data(open).buffer_c, 0);
  opeu_open_clear_screen(open, event);
  Call(open, event, PALETTE_SET, Palette(open, 0x2e));
  Call(open, event, BLIT_FAR, 0, 0x50, 0x50, 0xa0, 0, Data(open).buffer_d, 0);
  i = g = Data(open).bob_b[0];
  f = e = j = h = a = 0;
  for (;; f++) {
    d  = Call(open, event, FM_BUSY);
    d -= 12;
    g  = Data(open).bob_b[e];
    Call(open, event, BLIT_FAR, 0, g, 0x50, 0xa0, 0, (a != 0u) ? Data(open).buffer_d : Data(open).buffer_c, 0);
    Call(open, event, BLIT_FAR, 0x23, g + 0x60, 8, 0x40, 0, Data(open).buffer_a, h);
    a ^= 1;
    e++;
    if (e > 19) e = 0;
    i = Data(open).bob_b[e];
    if (i < g) Call(open, event, UI_BOX, 0, i + 0xa0, 0x50, g - i, 0);
    b = f % 10;
    if (b > 4 && b < 9) {
      Call(open, event, BLIT_FAR, Data(open).flak_x[f / 10 % 4] - b + 5, Data(open).flak_y[f / 10 % 4], 6, 0x3c, 0, Data(open).buffer_b, j);
      j += 0x5a0;
      if (j >= 0x1680) j = 0;
    }
    h += 0x800;
    if (h >= 0x2800) h = 0;
    if ((f >= 30) && (Call(open, event, PALETTE_STEP, Palette(open, 0)) == 0u)) break;

    while (Call(open, event, FM_BUSY) > d) {
      if (Call(open, event, INPUT_POLL) != 0u) return 1;
    }
  }
  if (Call(open, event, FADE_OUT) != 0u) return 1;
  return 0;
}

auto opeu_open_scene_flak(Open& open, OpenEvent event) -> int {
  if (opeu_open_flash_preload(open, event) != 0) return 1;
  if (opeu_open_bomber_flak(open, event) != 0) return 1;
  return 0;
}

/* p: column / flip count, toggle: figure frame, k: salvo, o: column offset. */
auto opeu_open_gun_setup(Open& open, OpenEvent event) -> int {
  int      p      = 0;
  int      toggle = 0;
  int      k      = 0;
  uint16_t o      = 0;
  opeu_open_clear_screen(open, event);
  PictureAt(open, event, 0, 0, 0x5eddeL);
  Call(open, event, BITMAP_SAVE, 0, 0, 6, 0x30, Data(open).buffer_a, 0);
  Call(open, event, BITMAP_SAVE, 6, 0, 6, 0x30, Data(open).buffer_a, 0x480);
  Call(open, event, BITMAP_SAVE, 0xc, 0, 6, 0x30, Data(open).buffer_a, 0x900);
  Call(open, event, BITMAP_SAVE, 0x12, 0, 6, 0x30, Data(open).buffer_a, 0xd80);
  opeu_open_clear_screen(open, event);
  Call(open, event, BLIT_FAR, 0, 0x108, 6, 0x30, 0, Data(open).buffer_a, 0x900);
  Call(open, event, BLIT_FAR, 6, 0x108, 6, 0x30, 0, Data(open).buffer_a, 0x900);
  Call(open, event, BLIT_FAR, 0xc, 0x108, 6, 0x30, 0, Data(open).buffer_a, 0xd80);
  Call(open, event, BLIT_FAR, 0x12, 0x108, 6, 0x30, 0, Data(open).buffer_a, 0x480);
  Call(open, event, BLIT_FAR, 0x18, 0x108, 6, 0x30, 0, Data(open).buffer_a, 0x900);
  Call(open, event, BLIT_FAR, 0x1e, 0x108, 6, 0x30, 0, Data(open).buffer_a, 0x480);
  Call(open, event, BLIT_FAR, 0x24, 0x108, 6, 0x30, 0, Data(open).buffer_a, 0xd80);
  Call(open, event, BLIT_FAR, 0x28, 0x108, 6, 0x30, 0, Data(open).buffer_a, 0xd80);
  Call(open, event, BLIT_FAR, 0x30, 0x108, 6, 0x30, 0, Data(open).buffer_a, 0x900);
  Call(open, event, BLIT_FAR, 0x36, 0x108, 6, 0x30, 0, Data(open).buffer_a, 0);
  o = 0;
  for (p = 0x35; p >= 0; p--) {
    Call(open, event, BITMAP_SAVE, p, 0x108, 1, 0x30, Data(open).buffer_b, o);
    o += 0xc0;
  }
  opeu_open_clear_screen(open, event);
  PictureAt(open, event, 0, 0x88, 0x5f135L);
  if (opeu_open_wait(open, event, 1) != 0) return 1;
  Call(open, event, BLIT_FAR, 0x28, 0x108, 6, 0x30, 0, Data(open).buffer_a, 0x900);
  o = 0;
  for (p = 0x27; p >= 0; p--) {
    Call(open, event, BITMAP_SAVE, p, 0x88, 1, 0xe0, Data(open).buffer_c, o);
    o += 0x380;
  }
  opeu_open_clear_screen(open, event);
  if (opeu_open_fade_to_palette(open, event, 0x2f, 2) != 0) return 1;
  Call(open, event, BLIT_FAR, 0x18, 0x58, 0x20, 0xe0, 0, Data(open).buffer_e, 0);
  Call(open, event, BLIT_FAR, 0x12, 0x108, 6, 0x30, 0, Data(open).buffer_a, 0);
  Call(open, event, BLIT_FAR, 0xc, 0x108, 6, 0x30, 0, Data(open).buffer_a, 0x900);
  Call(open, event, BLIT_FAR, 6, 0x108, 6, 0x30, 0, Data(open).buffer_a, 0x480);
  Call(open, event, BLIT_FAR, 0, 0x108, 6, 0x30, 0, Data(open).buffer_a, 0);
  Call(open, event, BITMAP_SAVE, 1, 0x58, 0x36, 0xe0, Data(open).buffer_d, 0);
  if (opeu_open_wait(open, event, 100) != 0) return 1;
  if (opeu_open_wait_until(open, event, 0xb121) != 0) return 1;
  for (k = toggle = 0; k < 3; k++) {
    for (p = 0; p < Data(open).gun_flips[k]; p++) {
      Call(open, event, BLIT_FAR, 0x18, 0x58, 0x20, 0xe0, 0, Data(open).buffer_e, (toggle != 0) ? 0 : 0x7000);
      if (opeu_open_wait(open, event, (toggle != 0) ? 2 : 4) != 0) return 1;
      toggle ^= 1;
    }
    if (opeu_open_wait(open, event, 100) != 0) return 1;
  }
  return 0;
}

auto opeu_open_gun_pan(Open& open, OpenEvent event) -> int {
  int      i   = 0;
  uint16_t off = 0;
  for (off = 0, i = 0; i < 0x36; i++) {
    Call(open, event, SCREEN_COPY, 0, 0x58, 0x36, 0xe0, 1, 0x58);
    Call(open, event, BLIT_FAR, 0, 0x108, 1, 0x30, 0, Data(open).buffer_b, off);
    off += 0xc0;
    if (Call(open, event, INPUT_POLL) != 0u) return 1;
  }
  for (off = 0, i = 0; i < 0x28; i++) {
    Call(open, event, SCREEN_COPY, 0, 0x58, 0x36, 0x110, 1, 0x58);
    Call(open, event, BLIT_FAR, 0, 0x88, 1, 0xe0, 0, Data(open).buffer_c, off);
    off += 0x380;
    if (Call(open, event, INPUT_POLL) != 0u) return 1;
  }
  if (opeu_open_wait(open, event, 15) != 0) return 1;
  if (opeu_open_wait_until(open, event, 0xac2d) != 0) return 1;
  return 0;
}

void opeu_open_flash_toggle(Open& open, OpenEvent event) {
  if (Data(open).flash_on != 0) {
    if (Data(open).flash_phase != 0)
      Call(open, event, PALETTE_SET, Palette(open, 0x2f));
    else
      Call(open, event, PALETTE_SET, Palette(open, 0x30));
    Data(open).flash_phase ^= 1;
  }
}

auto opeu_open_shell_run(Open& open, OpenEvent event, int first) -> int {
  int i = 0;
  for (i = first; i < first + 3; i++) {
    Call(open, event, DRAW_FAR, Data(open).shell_x[i], Data(open).shell_y[i], Data(open).shell_w[i], Data(open).shell_h[i], 0x407, Data(open).buffer_h, Data(open).shell_sprite[i]);
    if (opeu_open_wait(open, event, 1) != 0) return 1;
    Call(open, event, BLIT_FAR, Data(open).shell_x[i], Data(open).shell_y[i], Data(open).shell_ws[i], Data(open).shell_h[i], 0, Data(open).buffer_a, Data(open).shell_save[i]);
    opeu_open_flash_toggle(open, event);
  }
  return 0;
}

auto opeu_open_shell_run_pair(Open& open, OpenEvent event) -> int {
  int i = 0;
  int j = 0;
  j     = 3;
  for (i = 0; i < 3; i++, j++) {
    Call(open, event, DRAW_FAR, Data(open).shell_x[i], Data(open).shell_y[i], Data(open).shell_w[i], Data(open).shell_h[i], 0x407, Data(open).buffer_h, Data(open).shell_sprite[i]);
    Call(open, event, DRAW_FAR, Data(open).shell_x[j], Data(open).shell_y[j], Data(open).shell_w[j], Data(open).shell_h[j], 0x407, Data(open).buffer_h, Data(open).shell_sprite[j]);
    if (opeu_open_wait(open, event, 1) != 0) return 1;
    Call(open, event, BLIT_FAR, Data(open).shell_x[i], Data(open).shell_y[i], Data(open).shell_ws[i], Data(open).shell_h[i], 0, Data(open).buffer_a, Data(open).shell_save[i]);
    Call(open, event, BLIT_FAR, Data(open).shell_x[j], Data(open).shell_y[j], Data(open).shell_ws[j], Data(open).shell_h[j], 0, Data(open).buffer_a, Data(open).shell_save[j]);
    opeu_open_flash_toggle(open, event);
  }
  return 0;
}

/* runs: pair salvos left, frame: frame, shot: 1 first, 2 pairs, 3 last. */
// The opening's recovered animation script; it goes
// when the opening becomes a scene (M6).
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
auto opeu_open_gun_fire(Open& open, OpenEvent event) -> int {
  int runs  = 0;
  int frame = 0;
  int shot  = 0;
  for (frame = 0; frame < 6; frame++)
    Call(open, event, BITMAP_SAVE, Data(open).shell_x[frame], Data(open).shell_y[frame], Data(open).shell_ws[frame], Data(open).shell_h[frame], Data(open).buffer_a, Data(open).shell_save[frame]);
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
    if (frame == 3 || frame == 12 || frame == 20 || frame == 23 || frame == 40 || frame == 49) Data(open).flash_on ^= 1;
    if (shot == 1) {
      opeu_open_shell_run(open, event, 0);
      shot = 2;
    } else if (shot == 2) {
      opeu_open_shell_run_pair(open, event);
      if (--runs == 0) shot = 3;
    } else if (shot == 3) {
      opeu_open_shell_run(open, event, 3);
      shot = 0;
    }
    if (Data(open).flash_on == 0)
      Call(open, event, PALETTE_SET, Palette(open, 0x2f));
    else
      opeu_open_flash_toggle(open, event);
    if (opeu_open_wait(open, event, 1) != 0) return 1;
  }
  if (opeu_open_wait(open, event, 0x50) != 0) return 1;
  if (opeu_open_wait_until(open, event, 0xa8e2) != 0) return 1;
  if (Call(open, event, FADE_OUT) != 0u) return 1;
  return 0;
}

auto opeu_open_scene_artillery(Open& open, OpenEvent event) -> int {
  if (opeu_open_gun_setup(open, event) != 0) return 1;
  if (opeu_open_gun_pan(open, event) != 0) return 1;
  if (opeu_open_gun_fire(open, event) != 0) return 1;
  return 0;
}

auto opeu_open_title(Open& open, OpenEvent event) -> int {
  int i     = 0;
  int start = 0;
  opeu_open_clear_screen(open, event);
  start = (Data(open).aborted != 0) ? 0x65 : 0x31;
  Call(open, event, FADE_OUT_WAIT);
  opeu_open_fade_to_palette(open, event, 0, 2);
  Call(open, event, FADE_OUT);
  if (Data(open).aborted != 0) {
    opeu_open_fade_to_palette(open, event, 0x6f, 2);
    opeu_open_delay(open, event, 100);
  }
  PictureAt(open, event, 0x37, 0x3c, 0x5ff59L);
  for (i = start; i <= 0x6f; i++) {
    opeu_open_fade_to_palette(open, event, static_cast<uint8_t>(i), 2);
    if (Data(open).aborted != 0)
      opeu_open_delay(open, event, 15);
    else
      opeu_open_delay(open, event, 2);
  }
  opeu_open_delay(open, event, 60);
  PictureAt(open, event, 0x110, 0x148, 0x63a2cL);
  if (opeu_open_wait(open, event, 0x103) != 0) return 1;
  if (opeu_open_wait_until(open, event, 0xdced) != 0) return 1;
  MusicCommand4(open, event, 0);
  while (Call(open, event, MUSIC_PLAYING) != 0u) {
    if (Call(open, event, INPUT_POLL) != 0u) return 1;
  }
  if (Call(open, event, FADE_OUT) != 0u) return 1;
  opeu_open_clear_screen(open, event);
  return 0;
}

void opeu_open_init(Open& open, OpenEvent event) {
  int handle = 0;
  if ((Data(open).buffer_g = Call(open, event, DOS_ALLOC, 0x501)) == 0) {
    Call(open, event, CURSOR_PRINT, Near(open, Data(open).alloc_err1));
    Hang(open, event);
  }
  if ((Data(open).buffer_h = Call(open, event, DOS_ALLOC, 0x6d5)) == 0) {
    Call(open, event, CURSOR_PRINT, Near(open, Data(open).alloc_err2));
    Hang(open, event);
  }
  if ((Data(open).buffer_a = Call(open, event, DOS_ALLOC, 0x1000)) == 0) {
    Call(open, event, CURSOR_PRINT, Near(open, Data(open).alloc_err3));
    Hang(open, event);
  }
  if ((Data(open).buffer_b = Call(open, event, DOS_ALLOC, 0x1000)) == 0) {
    Call(open, event, CURSOR_PRINT, Near(open, Data(open).alloc_err4));
    Hang(open, event);
  }
  if ((Data(open).buffer_c = Call(open, event, DOS_ALLOC, 0x1000)) == 0) {
    Call(open, event, CURSOR_PRINT, Near(open, Data(open).alloc_err5));
    Hang(open, event);
  }
  if ((Data(open).buffer_d = Call(open, event, DOS_ALLOC, 0x1000)) == 0) {
    Call(open, event, CURSOR_PRINT, Near(open, Data(open).alloc_err6));
    Hang(open, event);
  }
  if ((Data(open).buffer_e = Call(open, event, DOS_ALLOC, 0x1000)) == 0) {
    Call(open, event, CURSOR_PRINT, Near(open, Data(open).alloc_err7));
    Hang(open, event);
  }
  if ((Data(open).buffer_f = Call(open, event, DOS_ALLOC, 0x1000)) == 0) {
    Call(open, event, CURSOR_PRINT, Near(open, Data(open).alloc_err9));
    Hang(open, event);
  }
  handle = Call(open, event, FILE_OR_DIE, Near(open, Data(open).name_mopengrp));
  Call(open, event, DOS_READ_FAR, handle, Data(open).buffer_h, 0, 0x6d40);
  Call(open, event, DOS_CLOSE, handle);
  Call(open, event, STUB_09B7, 1, 0);
  Call(open, event, VGA_CLEAR);
  Call(open, event, UI_TEXT, Near(open, Data(open).text_console));
  Call(open, event, IDENTITY_PALETTE);
  Call(open, event, SOUND_INITIALIZE);
  Call(open, event, MOUSE_RESET);
}

void opeu_open_free_buffers(Open& open, OpenEvent event) {
  Call(open, event, VGA_CLEAR);
  Call(open, event, DOS_FREE, Data(open).buffer_g);
  Call(open, event, DOS_FREE, Data(open).buffer_h);
  Call(open, event, DOS_FREE, Data(open).buffer_a);
  Call(open, event, DOS_FREE, Data(open).buffer_b);
  Call(open, event, DOS_FREE, Data(open).buffer_c);
  Call(open, event, DOS_FREE, Data(open).buffer_d);
  Call(open, event, DOS_FREE, Data(open).buffer_e);
  Call(open, event, DOS_FREE, Data(open).buffer_f);
}

/* Music driver (int 66h): AH=1 AL=5, then AH=2 AL=20. */
void opeu_open_shutdown(Open& open, OpenEvent event) {
  Regs regs{};
  Call(open, event, FADE_OUT_WAIT);
  regs.h.ah = 1;
  regs.h.al = 5;
  Int86(open, event, 0x66, regs);
  regs.h.ah = 2;
  regs.h.al = 20;
  Int86(open, event, 0x66, regs);
  Call(open, event, VGA_CLEAR);
  Call(open, event, FADE_ALT);
  while (Call(open, event, DOS_EXISTS, Near(open, Data(open).name_scesel)) == 0u);
  opeu_open_free_buffers(open, event);
}

void opeu_open_files_open(Open& open, OpenEvent event) {
  Data(open).picture_handle = static_cast<int16_t>(Call(open, event, FILE_OR_DIE, Near(open, Data(open).name_npk)));
  Data(open).logo_file      = static_cast<int16_t>(Call(open, event, FILE_OR_DIE, Near(open, Data(open).name_fnt)));
}

void opeu_open_files_close(Open& open, OpenEvent event) {
  Call(open, event, DOS_CLOSE, Data(open).picture_handle);
  Call(open, event, DOS_CLOSE, Data(open).logo_file);
}

void opeu_open_main(Open& open, OpenEvent event) {
  Call(open, event, INSTALL_DOS_HANDLERS);
  opeu_open_init(open, event);
  opeu_open_files_open(open, event);
  for (;;) {
    Call(open, event, TIMELINE_START, 20000);
    opeu_open_scene_koei(open, event);
    if (opeu_open_scene_prologue(open, event) != 0) {
      Data(open).aborted = 1;
      break;
    }
    if (opeu_open_scene_color_cycle(open, event) != 0) {
      Data(open).aborted = 1;
      break;
    }
    if (opeu_open_scene_flicker(open, event) != 0) {
      Data(open).aborted = 1;
      break;
    }
    if (opeu_open_scene_panorama(open, event) != 0) {
      Data(open).aborted = 1;
      break;
    }
    if (opeu_open_scene_units(open, event) != 0) {
      Data(open).aborted = 1;
      break;
    }
    if (opeu_open_scene_bombing(open, event) != 0) {
      Data(open).aborted = 1;
      break;
    }
    if (opeu_open_scene_flak(open, event) != 0) {
      Data(open).aborted = 1;
      break;
    }
    if (opeu_open_scene_artillery(open, event) != 0) {
      Data(open).aborted = 1;
      break;
    }
    if (opeu_open_title(open, event) != 0) {
      Data(open).aborted = 0;
      break;
    }
  }
  if (Data(open).aborted != 0) opeu_open_title(open, event);
  opeu_open_files_close(open, event);
  opeu_open_shutdown(open, event);
  Call(open, event, FM_NOOP);
}

}
