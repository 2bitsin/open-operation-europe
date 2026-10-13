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
 * The other modules' routines and the services (display, files, the FM
 * driver) are called directly; far buffers are reached through open-far.hpp.
 * Near pointers are DS offsets; all arithmetic wraps at 16 bits.
 */
#include "recovered.hpp"

#include "open-far.hpp"
#include "open-services.hpp"
#include "process.hpp"
#include "services.hpp"
#include "vgalib.hpp"

#include <array>
#include <cstdint>

namespace opeu::intro::detail::openmain {
namespace {
/* A picture of OPENGRP.NPK by its absolute file position. */
auto PictureAt(Open& open, uint16_t x, uint16_t y, uint32_t position) -> void {
  opeu_picture_file_at(open, x, y, static_cast<int32_t>(position));
}

/* Palette n of the table at DS:0444. */
auto PaletteNumber(Open& open, unsigned n) -> Palette {
  return Data(open).palettes[n];
}

/* Whether the file named at a near pointer exists. The shutdown busy-waits
 * for SCESEL.DAT with it; the host stays responsive while the file is
 * missing (no effect on the result). */
auto FileExists(Open const& open, uint16_t name) -> bool {
  auto const found{ opeu_dos_exists(reinterpret_cast<char const*>(&Byte(open, name))) }; // NOLINT(*-reinterpret-cast): the DOS name's characters
  if (found == 0) port_pump();
  return found != 0;
}
} // namespace

auto opeu_open_wait(uint16_t ticks) -> int;
auto opeu_open_wait_until(uint16_t target) -> int;
void opeu_open_clear_screen();
auto opeu_open_fade_to_palette(Open& open, uint8_t slot, int mode) -> int;
void opeu_open_load_koei_logo(Open& open);
void opeu_open_show_koei_logo(Open& open);
void opeu_open_show_logo_caption_a(Open& open);
void opeu_open_show_logo_caption_b(Open& open);
void opeu_open_scene_koei(Open& open);
auto opeu_open_prologue_pictures(Open& open) -> int;
auto opeu_open_scene_prologue(Open& open) -> int;
auto opeu_open_descend_animation(Open& open) -> int;
auto opeu_open_scene_color_cycle(Open& open) -> int;
auto opeu_open_scene_flicker(Open& open) -> int;
auto opeu_open_sprite_march(Open& open) -> int;
auto opeu_open_panorama_scroll(Open& open) -> int;
auto opeu_open_panorama_flashes(Open& open) -> int;
auto opeu_open_panorama_exit(Open& open) -> int;
auto opeu_open_scene_panorama(Open& open) -> int;
auto opeu_open_units_drive_in(Open& open) -> int;
auto opeu_open_units_animate(Open& open) -> int;
auto opeu_open_scene_units(Open& open) -> int;
auto opeu_open_bomber_approach(Open& open) -> int;
auto opeu_open_bomb_explosions(Open& open) -> int;
auto opeu_open_scene_bombing(Open& open) -> int;
auto opeu_open_flash_preload(Open& open) -> int;
auto opeu_open_bomber_flak(Open& open) -> int;
auto opeu_open_scene_flak(Open& open) -> int;
auto opeu_open_gun_setup(Open& open) -> int;
auto opeu_open_gun_pan(Open& open) -> int;
void opeu_open_flash_toggle(Open& open);
auto opeu_open_shell_run(Open& open, int first) -> int;
auto opeu_open_shell_run_pair(Open& open) -> int;
auto opeu_open_gun_fire(Open& open) -> int;
auto opeu_open_scene_artillery(Open& open) -> int;
auto opeu_open_title(Open& open) -> int;
void opeu_open_init(Open& open);
void opeu_open_free_buffers(Open& open);
void opeu_open_shutdown(Open& open);
void opeu_open_files_open(Open& open);
void opeu_open_files_close(Open& open);

void opeu_open_delay(uint16_t ticks) {
  if (ticks != 0u) {
    opeu_open_sfx_play(ticks);
    while (opeu_open_sfx_busy() != 0u);
  }
}

auto opeu_open_wait(uint16_t ticks) -> int {
  uint16_t target  = 0;
  target           = opeu_fm_busy();
  target          -= ticks;
  do {
    if (opeu_open_input_poll() != 0) return 1;
  } while (opeu_fm_busy() > target);
  return 0;
}

auto opeu_open_wait_until(uint16_t target) -> int {
  while (opeu_fm_busy() > target) {
    if (opeu_open_input_poll() != 0) return 1;
  }
  return 0;
}

void opeu_open_clear_screen() {
  opeu_vga_clear();
}

auto opeu_open_fade_to_palette(Open& open, uint8_t slot, int mode) -> int {
  return opeu_open_palette_fade(open, PaletteNumber(open, slot), static_cast<uint16_t>(mode));
}

void opeu_open_load_koei_logo(Open& open) {
  opeu_dos_seek(Data(open).logo_file, 0L, 0);
  ReadFar(open, static_cast<int16_t>(Data(open).logo_file), { Data(open).buffer_a, 0, 0xb60 });
  opeu_dos_seek(Data(open).logo_file, 0x1860L, 0);
  ReadFar(open, static_cast<int16_t>(Data(open).logo_file), { Data(open).buffer_a, 0xb60, 0x270 });
}

void opeu_open_show_koei_logo(Open& open) {
  opeu_open_clear_screen();
  opeu_open_fade_to_palette(open, 2, -1);
  DrawFar(open, { 0x1b, 0x60, 0x1a, 0x70 }, { Data(open).buffer_a, 0, 2 });
  opeu_open_delay(60);
  DrawFar(open, { 0x1b, 0xe0, 0x1a, 0x18 }, { Data(open).buffer_a, 0xb60, 7 });
  opeu_open_delay(60);
  opeu_open_wait_until(0xe98a);
  opeu_open_fade_in_wait(open);
}

void opeu_open_show_logo_caption_a(Open& open) {
  opeu_open_clear_screen();
  opeu_open_fade_to_palette(open, 2, -1);
  PictureAt(open, 0x80, 0xbc, 0L);
  opeu_open_delay(0x78);
  opeu_open_wait_until(0xe868);
  opeu_open_fade_in_wait(open);
}

void opeu_open_show_logo_caption_b(Open& open) {
  opeu_open_clear_screen();
  opeu_open_fade_to_palette(open, 2, -1);
  PictureAt(open, 0xf2, 0x76, 0x7f0L);
  PictureAt(open, 0x80, 0xba, 0x42dL);
  opeu_open_delay(0x78);
  opeu_open_wait_until(0xe50c);
  opeu_open_fade_in_wait(open);
}

void opeu_open_scene_koei(Open& open) {
  opeu_open_load_koei_logo(open);
  opeu_open_show_koei_logo(open);
  opeu_open_show_logo_caption_a(open);
  opeu_open_show_logo_caption_b(open);
}

auto opeu_open_prologue_pictures(Open& open) -> int {
  opeu_open_clear_screen();
  PictureAt(open, 0, 0, 0x2955fL);
  SaveScreen(open, { 0, 0, 0x50, 0x8c }, { Data(open).buffer_e, 0 });
  SaveScreen(open, { 0, 0x8c, 0x50, 0x8c }, { Data(open).buffer_f, 0 });
  opeu_open_clear_screen();
  PictureAt(open, 0x58, 0, 0xa81L);
  if (opeu_open_fade_to_palette(open, 3, 2) != 0) return 1;
  SaveScreen(open, { 10, 0, 0x3c, 0xc8 }, { Data(open).buffer_b, 0 });
  SaveScreen(open, { 10, 0xc8, 0x3c, 0xc8 }, { Data(open).buffer_c, 0 });
  if (opeu_open_wait(0x105) != 0) return 1;
  if (opeu_open_wait_until(0xe25d) != 0) return 1;
  if (opeu_open_fade_in(open) != 0) return 1;
  return 0;
}

auto opeu_open_scene_prologue(Open& open) -> int {
  if (opeu_open_wait_until(0xe4bf) != 0) return 1;
  opeu_music_start(open, 0);
  opeu_finish_side_initialization(open, 1);
  if (opeu_open_prologue_pictures(open) != 0) return 1;
  opeu_open_clear_screen();
  PictureAt(open, 0x90, 0x3c, 0x93f7L);
  if (opeu_open_fade_to_palette(open, 4, 2) != 0) return 1;
  SaveScreen(open, { 0x12, 0x3c, 0x30, 0x154 }, { Data(open).buffer_d, 0 });
  if (opeu_open_wait(0xdc) != 0) return 1;
  if (opeu_open_wait_until(0xdfa9) != 0) return 1;
  if (opeu_open_fade_in(open) != 0) return 1;
  return 0;
}

auto opeu_open_descend_animation(Open& open) -> int {
  uint16_t h     = 0;
  int      i     = 0;
  uint16_t off_a = 0;
  uint16_t off_b = 0;
  opeu_open_clear_screen();
  if (opeu_open_fade_to_palette(open, 30, 2) != 0) return 1;
  h     = 0;
  off_a = 0x2580;
  off_b = 0x4a40;
  for (i = 0; i < 25; i++) {
    BlitFar(open, { 0x22, 0, 12, h += 2 }, { Data(open).buffer_a, (off_a -= 0x60), 0 });
    if (opeu_open_wait(6) != 0) return 1;
    if (i > 21) opeu_open_palette_step(open, PaletteNumber(open, 0));
    BlitFar(open, { 0x22, 0, 12, h += 2 }, { Data(open).buffer_a, (off_a -= 0x60), 0 });
    if (opeu_open_wait(6) != 0) return 1;
    if (i > 21) opeu_open_palette_step(open, PaletteNumber(open, 0));
    off_a -= 0xc0;
    BlitFar(open, { 0x22, 0, 12, h += 2 }, { Data(open).buffer_a, (off_b -= 0x60), 0 });
    if (opeu_open_wait(6) != 0) return 1;
    if (i > 21) opeu_open_palette_step(open, PaletteNumber(open, 0));
    BlitFar(open, { 0x22, 0, 12, h += 2 }, { Data(open).buffer_a, (off_b -= 0x60), 0 });
    if (opeu_open_wait(6) != 0) return 1;
    if (i > 21) opeu_open_palette_step(open, PaletteNumber(open, 0));
    off_b -= 0xc0;
  }
  if (opeu_open_fade_in(open) != 0) return 1;
  return 0;
}

auto opeu_open_scene_color_cycle(Open& open) -> int {
  uint8_t pal = 0;
  int     j   = 0;
  opeu_open_clear_screen();
  PictureAt(open, 0x58, 0, 0x121a0L);
  if (opeu_open_fade_to_palette(open, 5, 2) != 0) return 1;
  if (opeu_open_wait(0x103) != 0) return 1;
  if (opeu_open_wait_until(0xdced) != 0) return 1;
  opeu_open_palette_set(open, PaletteNumber(open, 6));
  if (opeu_open_wait(5) != 0) return 1;
  opeu_open_palette_set(open, PaletteNumber(open, 7));
  if (opeu_open_wait(5) != 0) return 1;
  opeu_open_palette_set(open, PaletteNumber(open, 8));
  if (opeu_open_wait(5) != 0) return 1;
  opeu_open_palette_set(open, PaletteNumber(open, 9));
  if (opeu_open_wait(5) != 0) return 1;
  opeu_open_palette_set(open, PaletteNumber(open, 5));
  if (opeu_open_wait(0x14) != 0) return 1;
  if (opeu_open_fade_in(open) != 0) return 1;
  opeu_open_clear_screen();
  PictureAt(open, 0, 0, 0x28343L);
  PictureAt(open, 0x60, 0, 0x28c1bL);
  SaveScreen(open, { 0, 0, 12, 0xc8 }, { Data(open).buffer_a, 0 });
  SaveScreen(open, { 12, 0, 12, 0xc8 }, { Data(open).buffer_a, 0x2580 });
  opeu_open_clear_screen();
  PictureAt(open, 0, 0, 0x27b7dL);
  for (pal = 11, j = 0; j < 19; j++, pal++) {
    opeu_open_palette_set(open, PaletteNumber(open, pal));
    if (opeu_open_wait(5) != 0) return 1;
  }
  if (opeu_open_fade_to_palette(open, 0, 1) != 0) return 1;
  if (opeu_open_descend_animation(open) != 0) return 1;
  return 0;
}

auto opeu_open_scene_flicker(Open& open) -> int {
  int i   = 0;
  int off = 0;
  opeu_open_clear_screen();
  BlitFar(open, { 0, 0x3c, 0x50, 0x8c }, { Data(open).buffer_e, 0, 0 });
  BlitFar(open, { 0, 0xc8, 0x50, 0x8c }, { Data(open).buffer_f, 0, 0 });
  if (opeu_open_fade_to_palette(open, 31, 2) != 0) return 1;
  for (i = 0; i < 2; i++) {
    opeu_open_palette_set(open, PaletteNumber(open, 32));
    if (opeu_open_wait(6) != 0) return 1;
    opeu_open_palette_set(open, PaletteNumber(open, 31));
    if (opeu_open_wait(6) != 0) return 1;
  }
  for (i = 0; i < 10; i++) {
    opeu_open_palette_set(open, PaletteNumber(open, 32));
    if (opeu_open_wait(5) != 0) return 1;
    opeu_open_palette_set(open, PaletteNumber(open, 31));
    if (opeu_open_wait(8) != 0) return 1;
  }
  if (opeu_open_wait(0x78) != 0) return 1;
  if (opeu_open_fade_in(open) != 0) return 1;
  opeu_open_clear_screen();
  PictureAt(open, 0, 0, 0x399b9L);
  SaveScreen(open, { 0, 0, 0x12, 0x78 }, { Data(open).buffer_a, 0 });
  SaveScreen(open, { 0x12, 0, 0x12, 0x78 }, { Data(open).buffer_a, 0x21c0 });
  SaveScreen(open, { 0x24, 0, 0x12, 0x78 }, { Data(open).buffer_a, 0x4380 });
  SaveScreen(open, { 0, 0x78, 0x12, 0x78 }, { Data(open).buffer_a, 0x6540 });
  SaveScreen(open, { 0x12, 0x78, 0x12, 0x78 }, { Data(open).buffer_a, 0x8700 });
  SaveScreen(open, { 0x24, 0x78, 0x12, 0x78 }, { Data(open).buffer_a, 0xa8c0 });
  opeu_open_clear_screen();
  BlitFar(open, { 0x12, 0x3c, 0x30, 0x154 }, { Data(open).buffer_d, 0, 0 });
  if (opeu_open_wait(0x3c) != 0) return 1;
  PictureAt(open, 0xc, 0x3c, 0x37110L);
  SaveScreen(open, { 0, 0x3c, 0x26, 0x120 }, { Data(open).buffer_e, 0 });
  opeu_open_clear_screen();
  BlitFar(open, { 0x12, 0x3c, 0x30, 0x154 }, { Data(open).buffer_d, 0, 0 });
  if (opeu_open_fade_to_palette(open, 4, 2) != 0) return 1;
  if (opeu_open_wait_until(0xd56a) != 0) return 1;
  Dissolve(open, { 0, 0x3c }, { Data(open).buffer_e, 0, 0x0001 });
  Dissolve(open, { 0, 0x3c }, { Data(open).buffer_e, 0, 0x0101 });
  Dissolve(open, { 0, 0x3c }, { Data(open).buffer_e, 0, 0x1111 });
  Dissolve(open, { 0, 0x3c }, { Data(open).buffer_e, 0, 0x5555 });
  Dissolve(open, { 0, 0x3c }, { Data(open).buffer_e, 0, 0x7777 });
  Dissolve(open, { 0, 0x3c }, { Data(open).buffer_e, 0, 0x7f7f });
  Dissolve(open, { 0, 0x3c }, { Data(open).buffer_e, 0, 0xffff });
  if (opeu_open_wait(0xc8) != 0) return 1;
  if (opeu_open_wait_until(0xd3a2) != 0) return 1;
  if (opeu_open_fade_in(open) != 0) return 1;
  opeu_open_clear_screen();
  PictureAt(open, 0, 0, 0x3e98bL);
  for (off = 0, i = 78; i >= 0; i--) {
    SaveScreen(open, { i, 0, 1, 0xb0 }, { Data(open).buffer_d, static_cast<uint16_t>(off) });
    off += 0x2c0;
  }
  return 0;
}

auto opeu_open_sprite_march(Open& open) -> int {
  uint16_t d     = 0;
  int      pass  = 0;
  int      j     = 0;
  uint16_t off_a = 0;
  uint16_t off_b = 0;
  opeu_open_clear_screen();
  BlitFar(open, { 10, 0, 0x3c, 0xc8 }, { Data(open).buffer_b, 0, 0 });
  BlitFar(open, { 10, 0xc8, 0x3c, 0xc8 }, { Data(open).buffer_c, 0, 0 });
  if (opeu_open_fade_to_palette(open, 3, 2) != 0) return 1;
  if (opeu_open_wait_until(0xd2ed) != 0) return 1;
  for (d = 0x40, pass = 0;; d >>= 1, d++, pass++) {
    for (off_a = 0, off_b = 0x6540, j = 0; j < 3; j++) {
      BlitFar(open, { 10, 0x118, 0x12, 0x78 }, { Data(open).buffer_a, off_a, 0 });
      BlitFar(open, { 0x34, 0x118, 0x12, 0x78 }, { Data(open).buffer_a, off_b, 0 });
      off_a += 0x21c0;
      off_b += 0x21c0;
      if (opeu_open_wait(d) != 0) return 1;
    }
    if (pass > 40 && (opeu_open_palette_step(open, PaletteNumber(open, 0)) == 0)) {
      if (opeu_open_fade_in(open) != 0) return 1;
      break;
    }
  }
  return 0;
}

auto opeu_open_panorama_scroll(Open& open) -> int {
  int      i   = 0;
  uint16_t off = 0;
  opeu_open_clear_screen();
  PictureAt(open, 0, 0, 0x40bd5L);
  SaveScreen(open, { 0, 0, 0x20, 0xb0 }, { Data(open).buffer_a, 0 });
  if (opeu_open_wait(1) != 0) return 1;
  PictureAt(open, 0, 0, 0x421d5L);
  SaveScreen(open, { 0, 0, 0x20, 0xb0 }, { Data(open).buffer_b, 0 });
  if (opeu_open_wait(1) != 0) return 1;
  PictureAt(open, 0, 0, 0x43202L);
  SaveScreen(open, { 0, 0, 0x20, 0xb0 }, { Data(open).buffer_c, 0 });
  opeu_open_clear_screen();
  if (opeu_open_fade_to_palette(open, 33, 0) != 0) return 1;
  for (off = 0, i = 0; i < 0x4f; i++) {
    opeu_screen_copy(0x19, 0x70, 0x1d, 0xb0, 0x1a, 0x70);
    BlitFar(open, { 0x19, 0x70, 1, 0xb0 }, { Data(open).buffer_d, off, 0 });
    if (opeu_open_wait(5) != 0) return 1;
    off += 0x2c0;
  }
  SaveScreen(open, { 0x19, 0x70, 0x20, 0xb0 }, { Data(open).buffer_d, 0 });
  if (opeu_open_wait_until(0xcc15) != 0) return 1;
  return 0;
}

auto opeu_open_panorama_flashes(Open& open) -> int {
  int i = 0;
  for (i = 0; i < 2; i++) {
    opeu_open_palette_set(open, PaletteNumber(open, 34));
    opeu_open_palette_set(open, PaletteNumber(open, 35));
    if (opeu_open_wait(2) != 0) return 1;
    BlitFar(open, { 0x18, 0x70, 0x20, 0xb0 }, { Data(open).buffer_a, 0, 0 });
    opeu_open_palette_set(open, PaletteNumber(open, 33));
    if (opeu_open_wait(10) != 0) return 1;
    BlitFar(open, { 0x18, 0x70, 0x20, 0xb0 }, { Data(open).buffer_b, 0, 0 });
    if (opeu_open_wait(10) != 0) return 1;
    BlitFar(open, { 0x18, 0x70, 0x20, 0xb0 }, { Data(open).buffer_c, 0, 0 });
    if (opeu_open_wait(10) != 0) return 1;
    BlitFar(open, { 0x18, 0x70, 0x20, 0xb0 }, { Data(open).buffer_d, 0, 0 });
    if (i == 0 && (opeu_open_wait(100) != 0)) return 1;
  }
  if (opeu_open_wait(0x1e) != 0) return 1;
  if (opeu_open_wait_until(0xc9f8) != 0) return 1;
  return 0;
}

auto opeu_open_panorama_exit(Open& open) -> int {
  int i = 0;
  opeu_ui_box(0x38, 0x70, 1, 0xb0, 0);
  for (i = 0; i < 15; i++) {
    opeu_screen_copy(0x17, 0x70, 0x1f, 0xb0, 0x18, 0x70);
    if (opeu_open_wait(5) != 0) return 1;
  }
  if (opeu_open_wait_until(0xc99e) != 0) return 1;
  if (opeu_open_fade_in(open) != 0) return 1;
  PictureAt(open, 0, 0, 0x43fd2L);
  SaveScreen(open, { 0, 0, 0x14, 0xc0 }, { Data(open).buffer_a, 0 });
  SaveScreen(open, { 0x14, 0, 0x14, 0xc0 }, { Data(open).buffer_a, 0x3c00 });
  SaveScreen(open, { 0x28, 0, 0x14, 0xc0 }, { Data(open).buffer_a, 0x7800 });
  SaveScreen(open, { 0x3c, 0, 0x14, 0xc0 }, { Data(open).buffer_a, 0xb400 });
  SaveScreen(open, { 0, 0xc0, 0x14, 0xc0 }, { Data(open).buffer_b, 0 });
  SaveScreen(open, { 0x14, 0xc0, 0x14, 0xc0 }, { Data(open).buffer_b, 0x3c00 });
  SaveScreen(open, { 0x28, 0xc0, 0x14, 0xc0 }, { Data(open).buffer_b, 0x7800 });
  SaveScreen(open, { 0x3c, 0xc0, 0x14, 0xc0 }, { Data(open).buffer_b, 0xb400 });
  return 0;
}

auto opeu_open_scene_panorama(Open& open) -> int {
  if (opeu_open_sprite_march(open) != 0) return 1;
  if (opeu_open_panorama_scroll(open) != 0) return 1;
  if (opeu_open_panorama_flashes(open) != 0) return 1;
  if (opeu_open_panorama_exit(open) != 0) return 1;
  return 0;
}
/* x: strip column, off_a/off_b: strip offsets in buffer C. */
auto opeu_open_units_drive_in(Open& open) -> int {
  int      x     = 0;
  uint16_t off_a = 0;
  uint16_t off_b = 0;
  opeu_open_clear_screen();
  BlitFar(open, { 2, 0, 0x14, 0x60 }, { Data(open).buffer_a, 0, 0 });
  off_a = 0;
  for (x = 0x15; x > 0; x -= 2) {
    SaveScreen(open, { x, 0, 2, 0x60 }, { Data(open).buffer_c, off_a });
    off_a += 0x300;
  }
  for (x = 0x14; x >= 0; x -= 2) {
    SaveScreen(open, { x, 0, 2, 0x60 }, { Data(open).buffer_c, off_a });
    off_a += 0x300;
  }
  opeu_open_clear_screen();
  opeu_open_palette_set(open, PaletteNumber(open, 36));
  x = off_b = off_a = 0;
  for (; x < 31; x++) {
    opeu_screen_copy(0, 0xc4, 0x3c, 0x60, 2, 0xc4);
    if (x < 11) {
      BlitFar(open, { 0, 0xc4, 2, 0x60 }, { Data(open).buffer_c, off_a, 0 });
      off_a += 0x300;
    }
    if (x > 12 && x < 24) {
      BlitFar(open, { 0, 0xc4, 2, 0x60 }, { Data(open).buffer_c, off_b, 0 });
      off_b += 0x300;
    }
    if (opeu_open_wait(6) != 0) return 1;
  }
  if (opeu_open_wait(10) != 0) return 1;
  return 0;
}

/* i: frame count, j/k: frame of the right/left vehicle. */
auto opeu_open_units_animate(Open& open) -> int {
  int i = 0;
  int j = 0;
  int k = 0;
  i = k = j = 0;
  for (; i < 40; i++) {
    if (i == 0) {
      opeu_open_palette_set(open, PaletteNumber(open, 34));
      opeu_open_palette_set(open, PaletteNumber(open, 35));
      if (opeu_open_wait(2) != 0) return 1;
      opeu_open_palette_set(open, PaletteNumber(open, 36));
    }
    opeu_ui_box(0x2a, 0x64, 0x14, 0x80, 0);
    BlitFar(open, { 0x2a, Data(open).frame_y[j], 0x14, Data(open).frame_h[j] }, { j < 5 ? Data(open).buffer_a : Data(open).buffer_b, Data(open).frame_off[j], 0 });
    j++;
    if (j == 11) j = 8;
    opeu_ui_box(0x10, 0x64, 0x14, 0x80, 0);
    BlitFar(open, { 0x10, Data(open).frame_y[k], 0x14, Data(open).frame_h[k] }, { k < 5 ? Data(open).buffer_a : Data(open).buffer_b, Data(open).frame_off[k], 0 });
    if (i > 2) {
      k++;
      if (k == 11) k = 8;
    }
    if (opeu_open_wait(10) != 0) return 1;
    if (i > 30) opeu_open_palette_step(open, PaletteNumber(open, 0));
  }
  if (opeu_open_fade_in(open) != 0) return 1;
  if (opeu_open_wait_until(0xc5e2) != 0) return 1;
  return 0;
}

auto opeu_open_scene_units(Open& open) -> int {
  if (opeu_open_units_drive_in(open) != 0) return 1;
  if (opeu_open_units_animate(open) != 0) return 1;
  return 0;
}

/* a: composite toggle, b: frame deadline, c: bob index, d: frame,
 * e: y, f: faded in, g: rotating part offset, h: next y. */
auto opeu_open_bomber_approach(Open& open) -> int {
  uint16_t a = 0;
  uint16_t b = 0;
  int      c = 0;
  int      d = 0;
  uint16_t e = 0;
  int      f = 0;
  uint16_t g = 0;
  uint16_t h = 0;
  opeu_open_clear_screen();
  PictureAt(open, 0, 0, 0x46848L);
  SaveScreen(open, { 0, 0, 0x50, 0xa0 }, { Data(open).buffer_c, 0 });
  opeu_open_clear_screen();
  PictureAt(open, 0, 0, 0x487fbL);
  SaveScreen(open, { 0, 0, 8, 0x40 }, { Data(open).buffer_a, 0 });
  SaveScreen(open, { 8, 0, 8, 0x40 }, { Data(open).buffer_a, 0x800 });
  SaveScreen(open, { 0x10, 0, 8, 0x40 }, { Data(open).buffer_a, 0x1000 });
  SaveScreen(open, { 0x18, 0, 8, 0x40 }, { Data(open).buffer_a, 0x1800 });
  SaveScreen(open, { 0x20, 0, 8, 0x40 }, { Data(open).buffer_a, 0x2000 });
  PictureAt(open, 0, 0, 0x48ce9L);
  SaveScreen(open, { 0, 0, 0x32, 0x140 }, { Data(open).buffer_b, 0 });
  PictureAt(open, 0, 0, 0x4d847L);
  SaveScreen(open, { 0, 0, 0x1a, 0xb0 }, { Data(open).buffer_e, 0 });
  SaveScreen(open, { 0x1a, 0, 0x1a, 0xb0 }, { Data(open).buffer_e, 0x4780 });
  SaveScreen(open, { 0x34, 0, 0x1a, 0xb0 }, { Data(open).buffer_e, 0x8f00 });
  SaveScreen(open, { 0, 0xb0, 0x1a, 0xb0 }, { Data(open).buffer_f, 0 });
  SaveScreen(open, { 0x1a, 0xb0, 0x1a, 0xb0 }, { Data(open).buffer_f, 0x4780 });
  SaveScreen(open, { 0x34, 0xb0, 0x1a, 0xb0 }, { Data(open).buffer_f, 0x8f00 });
  opeu_open_clear_screen();
  PictureAt(open, 0, 0, 0x47428L);
  SaveScreen(open, { 0, 0, 0x10, 0x90 }, { Data(open).buffer_a, 0x2800 });
  SaveScreen(open, { 0x10, 0, 0x10, 0x90 }, { Data(open).buffer_a, 0x7000 });
  SaveScreen(open, { 0x20, 0, 0x10, 0x90 }, { Data(open).buffer_a, 0x4c00 });
  SaveScreen(open, { 0x30, 0, 0x10, 0x90 }, { Data(open).buffer_a, 0x9400 });
  opeu_open_clear_screen();
  BlitFar(open, { 0, 0x50, 0x50, 0xa0 }, { Data(open).buffer_c, 0, 0 });
  BlitFar(open, { 0x13, 0x50, 0x10, 0x90 }, { Data(open).buffer_a, 0x2800, 0 });
  BlitFar(open, { 0x32, 0x50, 0x10, 0x90 }, { Data(open).buffer_a, 0x4c00, 0 });
  BlitFar(open, { 0, 0xf0, 0x50, 0xa0 }, { Data(open).buffer_c, 0, 0 });
  BlitFar(open, { 0x13, 0xf0, 0x10, 0x90 }, { Data(open).buffer_a, 0x7000, 0 });
  BlitFar(open, { 0x32, 0xf0, 0x10, 0x90 }, { Data(open).buffer_a, 0x9400, 0 });
  SaveScreen(open, { 0, 0x50, 0x50, 0xa0 }, { Data(open).buffer_c, 0 });
  SaveScreen(open, { 0, 0xf0, 0x50, 0xa0 }, { Data(open).buffer_d, 0 });
  opeu_open_clear_screen();
  BlitFar(open, { 0, 0x50, 0x50, 0xa0 }, { Data(open).buffer_d, 0, 0 });
  h = e = Data(open).bob_a[0];
  d = f = c = a = g = 0;
  for (; d < 0x3a; d++) {
    b  = opeu_fm_busy();
    b -= 12;
    e  = Data(open).bob_a[c];
    BlitFar(open, { 0, e, 0x50, 0xa0 }, { (a != 0u) ? Data(open).buffer_d : Data(open).buffer_c, 0, 0 });
    BlitFar(open, { 0x23, e + 0x60, 8, 0x40 }, { Data(open).buffer_a, g, 0 });
    a ^= 1;
    c++;
    if (c > 19) c = 0;
    h = Data(open).bob_a[c];
    if (h < e) opeu_ui_box(0, h + 0xa0, 0x50, e - h, 0);
    g += 0x800;
    if (g >= 0x2800) g = 0;
    if ((f == 0) && (opeu_open_palette_step(open, PaletteNumber(open, 0x25)) == 0)) f = 1;

    while (opeu_fm_busy() > b) {
      if (opeu_open_input_poll() != 0) return 1;
    }
  }
  if (opeu_open_fade_in(open) != 0) return 1;
  return 0;
}

/* k: explosion, j: frame, n: palette step, pos: frame offset. */
// The opening's recovered animation script; it goes
// when the opening becomes a scene (M6).
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
auto opeu_open_bomb_explosions(Open& open) -> int {
  int      k   = 0;
  int      j   = 0;
  int      n   = 0;
  uint16_t pos = 0;
  opeu_open_clear_screen();
  opeu_open_palette_set(open, PaletteNumber(open, 0x26));
  BlitFar(open, { 0xf, 0x28, 0x32, 0x140 }, { Data(open).buffer_b, 0, 0 });
  if (opeu_open_wait_until(0xbd4b) != 0) return 1;
  if (opeu_open_wait(50) != 0) return 1;
  for (k = 0; k < 3; k++) {
    SaveScreen(open, { Data(open).blast_x[k], Data(open).blast_y[k], 0x1a, 0xb0 }, { Data(open).buffer_a, 0xb800 });
    for (pos = 0, j = 0; j < 3; j++, pos += 0x4780) {
      CopyFar(open, { Data(open).buffer_a, 0xb800 }, { Data(open).buffer_b, pos, 0x4780 });
      AndMask(open, { Data(open).buffer_b, pos, 0x1a, 0xb0 }, { Data(open).buffer_h, Data(open).blast_masks[j] });
      OrSprite(open, { Data(open).buffer_b, pos, 0x1a, 0xb0 }, { Data(open).buffer_e, Data(open).blast_sprites[j] });
    }
    for (pos = 0, j = 3; j < 6; j++, pos += 0x4780) {
      CopyFar(open, { Data(open).buffer_a, 0xb800 }, { Data(open).buffer_c, pos, 0x4780 });
      AndMask(open, { Data(open).buffer_c, pos, 0x1a, 0xb0 }, { Data(open).buffer_h, Data(open).blast_masks[j] });
      OrSprite(open, { Data(open).buffer_c, pos, 0x1a, 0xb0 }, { Data(open).buffer_f, Data(open).blast_sprites[j] });
    }
    for (pos = 0, j = 0; j < 6; j++) {
      BlitFar(open, { Data(open).blast_x[k], Data(open).blast_y[k], 0x1a, 0xb0 }, { j < 3 ? Data(open).buffer_b : Data(open).buffer_c, pos, 4 });
      if (pos == 0x8f00)
        pos = 0;
      else
        pos += 0x4780;
      for (n = 0; n < Data(open).blast_steps[j]; n++) opeu_open_palette_step(open, PaletteNumber(open, Data(open).blast_slots[k]));
      if (opeu_open_wait(2) != 0) return 1;
    }
    BlitFar(open, { Data(open).blast_x[k], Data(open).blast_y[k], 0x1a, 0xb0 }, { Data(open).buffer_a, 0xb800, 0 });
    if (opeu_open_fade_to_palette(open, 0x26, 1) != 0) return 1;
    BlitFar(open, { Data(open).blast_x[k], Data(open).blast_y[k], 0x1a, 0xb0 }, { Data(open).buffer_a, 0xb800, 0 });
    if (opeu_open_wait(10) != 0) return 1;
  }
  if (opeu_open_wait(90) != 0) return 1;
  if (opeu_open_wait_until(0xbada) != 0) return 1;
  if (opeu_open_fade_in(open) != 0) return 1;
  return 0;
}

auto opeu_open_scene_bombing(Open& open) -> int {
  if (opeu_open_bomber_approach(open) != 0) return 1;
  if (opeu_open_bomb_explosions(open) != 0) return 1;
  return 0;
}

auto opeu_open_flash_preload(Open& open) -> int {
  int i = 0;
  opeu_open_clear_screen();
  PictureAt(open, 0x4c, 0xc, 0x54d6dL);
  if (opeu_open_fade_to_palette(open, 0x2a, 2) != 0) return 1;
  if (opeu_open_wait(100) != 0) return 1;
  if (opeu_open_wait_until(0xb8d3) != 0) return 1;
  for (i = 0; i < 3; i++) {
    opeu_open_palette_set(open, PaletteNumber(open, Data(open).flash_slots[i]));
    while (opeu_open_palette_step(open, PaletteNumber(open, 0x2a)) != 0);
    if (opeu_open_wait(60) != 0) return 1;
  }
  if (opeu_open_wait(30) != 0) return 1;
  if (opeu_open_wait_until(0xb6f8) != 0) return 1;
  if (opeu_open_fade_in(open) != 0) return 1;
  opeu_open_clear_screen();
  PictureAt(open, 0, 0xa, 0x5b770L);
  SaveScreen(open, { 0, 0, 6, 0x3c }, { Data(open).buffer_b, 0 });
  SaveScreen(open, { 6, 0, 6, 0x3c }, { Data(open).buffer_b, 0x5a0 });
  SaveScreen(open, { 0xc, 0, 6, 0x3c }, { Data(open).buffer_b, 0xb40 });
  SaveScreen(open, { 0x12, 0, 6, 0x3c }, { Data(open).buffer_b, 0x10e0 });
  opeu_open_clear_screen();
  PictureAt(open, 0, 0, 0x5b9e3L);
  SaveScreen(open, { 0, 0, 0x20, 0xe0 }, { Data(open).buffer_e, 0 });
  PictureAt(open, 0, 0, 0x5d486L);
  SaveScreen(open, { 0, 0, 0x20, 0xe0 }, { Data(open).buffer_e, 0x7000 });
  if (opeu_open_wait(15) != 0) return 1;
  if (opeu_open_wait_until(0xb6f8) != 0) return 1;
  if (opeu_open_fade_in(open) != 0) return 1;
  return 0;
}

/* a: composite toggle, b: frame % 10, d: deadline, e: bob index,
 * f: frame, g: y, h: rotating part offset, i: next y, j: burst offset. */
auto opeu_open_bomber_flak(Open& open) -> int {
  uint16_t a = 0;
  int      b = 0;
  uint16_t d = 0;
  int      e = 0;
  int      f = 0;
  uint16_t g = 0;
  uint16_t h = 0;
  uint16_t i = 0;
  uint16_t j = 0;
  opeu_open_clear_screen();
  BlitFar(open, { 0, 0x50, 0x50, 0xa0 }, { Data(open).buffer_d, 0, 0 });
  BlitFar(open, { 0x13, 0x50, 0x10, 0x90 }, { Data(open).buffer_a, 0x2800, 0 });
  BlitFar(open, { 0x32, 0x50, 0x10, 0x90 }, { Data(open).buffer_a, 0x4c00, 0 });
  SaveScreen(open, { 0, 0x50, 0x50, 0xa0 }, { Data(open).buffer_c, 0 });
  opeu_open_clear_screen();
  opeu_open_palette_set(open, PaletteNumber(open, 0x2e));
  BlitFar(open, { 0, 0x50, 0x50, 0xa0 }, { Data(open).buffer_d, 0, 0 });
  i = g = Data(open).bob_b[0];
  f = e = j = h = a = 0;
  for (;; f++) {
    d  = opeu_fm_busy();
    d -= 12;
    g  = Data(open).bob_b[e];
    BlitFar(open, { 0, g, 0x50, 0xa0 }, { (a != 0u) ? Data(open).buffer_d : Data(open).buffer_c, 0, 0 });
    BlitFar(open, { 0x23, g + 0x60, 8, 0x40 }, { Data(open).buffer_a, h, 0 });
    a ^= 1;
    e++;
    if (e > 19) e = 0;
    i = Data(open).bob_b[e];
    if (i < g) opeu_ui_box(0, i + 0xa0, 0x50, g - i, 0);
    b = f % 10;
    if (b > 4 && b < 9) {
      BlitFar(open, { Data(open).flak_x[f / 10 % 4] - b + 5, Data(open).flak_y[f / 10 % 4], 6, 0x3c }, { Data(open).buffer_b, j, 0 });
      j += 0x5a0;
      if (j >= 0x1680) j = 0;
    }
    h += 0x800;
    if (h >= 0x2800) h = 0;
    if ((f >= 30) && (opeu_open_palette_step(open, PaletteNumber(open, 0)) == 0)) break;

    while (opeu_fm_busy() > d) {
      if (opeu_open_input_poll() != 0) return 1;
    }
  }
  if (opeu_open_fade_in(open) != 0) return 1;
  return 0;
}

auto opeu_open_scene_flak(Open& open) -> int {
  if (opeu_open_flash_preload(open) != 0) return 1;
  if (opeu_open_bomber_flak(open) != 0) return 1;
  return 0;
}

/* p: column / flip count, toggle: figure frame, k: salvo, o: column offset. */
auto opeu_open_gun_setup(Open& open) -> int {
  int      p      = 0;
  int      toggle = 0;
  int      k      = 0;
  uint16_t o      = 0;
  opeu_open_clear_screen();
  PictureAt(open, 0, 0, 0x5eddeL);
  SaveScreen(open, { 0, 0, 6, 0x30 }, { Data(open).buffer_a, 0 });
  SaveScreen(open, { 6, 0, 6, 0x30 }, { Data(open).buffer_a, 0x480 });
  SaveScreen(open, { 0xc, 0, 6, 0x30 }, { Data(open).buffer_a, 0x900 });
  SaveScreen(open, { 0x12, 0, 6, 0x30 }, { Data(open).buffer_a, 0xd80 });
  opeu_open_clear_screen();
  BlitFar(open, { 0, 0x108, 6, 0x30 }, { Data(open).buffer_a, 0x900, 0 });
  BlitFar(open, { 6, 0x108, 6, 0x30 }, { Data(open).buffer_a, 0x900, 0 });
  BlitFar(open, { 0xc, 0x108, 6, 0x30 }, { Data(open).buffer_a, 0xd80, 0 });
  BlitFar(open, { 0x12, 0x108, 6, 0x30 }, { Data(open).buffer_a, 0x480, 0 });
  BlitFar(open, { 0x18, 0x108, 6, 0x30 }, { Data(open).buffer_a, 0x900, 0 });
  BlitFar(open, { 0x1e, 0x108, 6, 0x30 }, { Data(open).buffer_a, 0x480, 0 });
  BlitFar(open, { 0x24, 0x108, 6, 0x30 }, { Data(open).buffer_a, 0xd80, 0 });
  BlitFar(open, { 0x28, 0x108, 6, 0x30 }, { Data(open).buffer_a, 0xd80, 0 });
  BlitFar(open, { 0x30, 0x108, 6, 0x30 }, { Data(open).buffer_a, 0x900, 0 });
  BlitFar(open, { 0x36, 0x108, 6, 0x30 }, { Data(open).buffer_a, 0, 0 });
  o = 0;
  for (p = 0x35; p >= 0; p--) {
    SaveScreen(open, { p, 0x108, 1, 0x30 }, { Data(open).buffer_b, o });
    o += 0xc0;
  }
  opeu_open_clear_screen();
  PictureAt(open, 0, 0x88, 0x5f135L);
  if (opeu_open_wait(1) != 0) return 1;
  BlitFar(open, { 0x28, 0x108, 6, 0x30 }, { Data(open).buffer_a, 0x900, 0 });
  o = 0;
  for (p = 0x27; p >= 0; p--) {
    SaveScreen(open, { p, 0x88, 1, 0xe0 }, { Data(open).buffer_c, o });
    o += 0x380;
  }
  opeu_open_clear_screen();
  if (opeu_open_fade_to_palette(open, 0x2f, 2) != 0) return 1;
  BlitFar(open, { 0x18, 0x58, 0x20, 0xe0 }, { Data(open).buffer_e, 0, 0 });
  BlitFar(open, { 0x12, 0x108, 6, 0x30 }, { Data(open).buffer_a, 0, 0 });
  BlitFar(open, { 0xc, 0x108, 6, 0x30 }, { Data(open).buffer_a, 0x900, 0 });
  BlitFar(open, { 6, 0x108, 6, 0x30 }, { Data(open).buffer_a, 0x480, 0 });
  BlitFar(open, { 0, 0x108, 6, 0x30 }, { Data(open).buffer_a, 0, 0 });
  SaveScreen(open, { 1, 0x58, 0x36, 0xe0 }, { Data(open).buffer_d, 0 });
  if (opeu_open_wait(100) != 0) return 1;
  if (opeu_open_wait_until(0xb121) != 0) return 1;
  for (k = toggle = 0; k < 3; k++) {
    for (p = 0; p < Data(open).gun_flips[k]; p++) {
      BlitFar(open, { 0x18, 0x58, 0x20, 0xe0 }, { Data(open).buffer_e, static_cast<uint16_t>((toggle != 0) ? 0 : 0x7000), 0 });
      if (opeu_open_wait((toggle != 0) ? 2 : 4) != 0) return 1;
      toggle ^= 1;
    }
    if (opeu_open_wait(100) != 0) return 1;
  }
  return 0;
}

auto opeu_open_gun_pan(Open& open) -> int {
  int      i   = 0;
  uint16_t off = 0;
  for (off = 0, i = 0; i < 0x36; i++) {
    opeu_screen_copy(0, 0x58, 0x36, 0xe0, 1, 0x58);
    BlitFar(open, { 0, 0x108, 1, 0x30 }, { Data(open).buffer_b, off, 0 });
    off += 0xc0;
    if (opeu_open_input_poll() != 0) return 1;
  }
  for (off = 0, i = 0; i < 0x28; i++) {
    opeu_screen_copy(0, 0x58, 0x36, 0x110, 1, 0x58);
    BlitFar(open, { 0, 0x88, 1, 0xe0 }, { Data(open).buffer_c, off, 0 });
    off += 0x380;
    if (opeu_open_input_poll() != 0) return 1;
  }
  if (opeu_open_wait(15) != 0) return 1;
  if (opeu_open_wait_until(0xac2d) != 0) return 1;
  return 0;
}

void opeu_open_flash_toggle(Open& open) {
  if (Data(open).flash_on != 0) {
    if (Data(open).flash_phase != 0)
      opeu_open_palette_set(open, PaletteNumber(open, 0x2f));
    else
      opeu_open_palette_set(open, PaletteNumber(open, 0x30));
    Data(open).flash_phase ^= 1;
  }
}

auto opeu_open_shell_run(Open& open, int first) -> int {
  int i = 0;
  for (i = first; i < first + 3; i++) {
    DrawFar(open, { Data(open).shell_x[i], Data(open).shell_y[i], Data(open).shell_w[i], Data(open).shell_h[i] }, { Data(open).buffer_h, Data(open).shell_sprite[i], 0x407 });
    if (opeu_open_wait(1) != 0) return 1;
    BlitFar(open, { Data(open).shell_x[i], Data(open).shell_y[i], Data(open).shell_ws[i], Data(open).shell_h[i] }, { Data(open).buffer_a, Data(open).shell_save[i], 0 });
    opeu_open_flash_toggle(open);
  }
  return 0;
}

auto opeu_open_shell_run_pair(Open& open) -> int {
  int i = 0;
  int j = 0;
  j     = 3;
  for (i = 0; i < 3; i++, j++) {
    DrawFar(open, { Data(open).shell_x[i], Data(open).shell_y[i], Data(open).shell_w[i], Data(open).shell_h[i] }, { Data(open).buffer_h, Data(open).shell_sprite[i], 0x407 });
    DrawFar(open, { Data(open).shell_x[j], Data(open).shell_y[j], Data(open).shell_w[j], Data(open).shell_h[j] }, { Data(open).buffer_h, Data(open).shell_sprite[j], 0x407 });
    if (opeu_open_wait(1) != 0) return 1;
    BlitFar(open, { Data(open).shell_x[i], Data(open).shell_y[i], Data(open).shell_ws[i], Data(open).shell_h[i] }, { Data(open).buffer_a, Data(open).shell_save[i], 0 });
    BlitFar(open, { Data(open).shell_x[j], Data(open).shell_y[j], Data(open).shell_ws[j], Data(open).shell_h[j] }, { Data(open).buffer_a, Data(open).shell_save[j], 0 });
    opeu_open_flash_toggle(open);
  }
  return 0;
}

/* runs: pair salvos left, frame: frame, shot: 1 first, 2 pairs, 3 last. */
// The opening's recovered animation script; it goes
// when the opening becomes a scene (M6).
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
auto opeu_open_gun_fire(Open& open) -> int {
  int runs  = 0;
  int frame = 0;
  int shot  = 0;
  for (frame = 0; frame < 6; frame++)
    SaveScreen(open, { Data(open).shell_x[frame], Data(open).shell_y[frame], Data(open).shell_ws[frame], Data(open).shell_h[frame] }, { Data(open).buffer_a, Data(open).shell_save[frame] });
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
      opeu_open_shell_run(open, 0);
      shot = 2;
    } else if (shot == 2) {
      opeu_open_shell_run_pair(open);
      if (--runs == 0) shot = 3;
    } else if (shot == 3) {
      opeu_open_shell_run(open, 3);
      shot = 0;
    }
    if (Data(open).flash_on == 0)
      opeu_open_palette_set(open, PaletteNumber(open, 0x2f));
    else
      opeu_open_flash_toggle(open);
    if (opeu_open_wait(1) != 0) return 1;
  }
  if (opeu_open_wait(0x50) != 0) return 1;
  if (opeu_open_wait_until(0xa8e2) != 0) return 1;
  if (opeu_open_fade_in(open) != 0) return 1;
  return 0;
}

auto opeu_open_scene_artillery(Open& open) -> int {
  if (opeu_open_gun_setup(open) != 0) return 1;
  if (opeu_open_gun_pan(open) != 0) return 1;
  if (opeu_open_gun_fire(open) != 0) return 1;
  return 0;
}

auto opeu_open_title(Open& open) -> int {
  int i     = 0;
  int start = 0;
  opeu_open_clear_screen();
  start = (Data(open).aborted != 0) ? 0x65 : 0x31;
  opeu_open_fade_in_wait(open);
  opeu_open_fade_to_palette(open, 0, 2);
  opeu_open_fade_in(open);
  if (Data(open).aborted != 0) {
    opeu_open_fade_to_palette(open, 0x6f, 2);
    opeu_open_delay(100);
  }
  PictureAt(open, 0x37, 0x3c, 0x5ff59L);
  for (i = start; i <= 0x6f; i++) {
    opeu_open_fade_to_palette(open, static_cast<uint8_t>(i), 2);
    if (Data(open).aborted != 0)
      opeu_open_delay(15);
    else
      opeu_open_delay(2);
  }
  opeu_open_delay(60);
  PictureAt(open, 0x110, 0x148, 0x63a2cL);
  if (opeu_open_wait(0x103) != 0) return 1;
  if (opeu_open_wait_until(0xdced) != 0) return 1;
  opeu_finish_side_initialization(open, 0);
  while (opeu_music_playing(open) != 0) {
    if (opeu_open_input_poll() != 0) return 1;
  }
  if (opeu_open_fade_in(open) != 0) return 1;
  opeu_open_clear_screen();
  return 0;
}

void opeu_open_init(Open& open) {
  int handle = 0;
  if ((Data(open).buffer_g = open_dos_alloc(0x501)) == 0) {
    CursorPrint(open, std::array<uint16_t, 1>{ Near(open, Data(open).alloc_err1) });
    Hang();
  }
  if ((Data(open).buffer_h = open_dos_alloc(0x6d5)) == 0) {
    CursorPrint(open, std::array<uint16_t, 1>{ Near(open, Data(open).alloc_err2) });
    Hang();
  }
  if ((Data(open).buffer_a = open_dos_alloc(0x1000)) == 0) {
    CursorPrint(open, std::array<uint16_t, 1>{ Near(open, Data(open).alloc_err3) });
    Hang();
  }
  if ((Data(open).buffer_b = open_dos_alloc(0x1000)) == 0) {
    CursorPrint(open, std::array<uint16_t, 1>{ Near(open, Data(open).alloc_err4) });
    Hang();
  }
  if ((Data(open).buffer_c = open_dos_alloc(0x1000)) == 0) {
    CursorPrint(open, std::array<uint16_t, 1>{ Near(open, Data(open).alloc_err5) });
    Hang();
  }
  if ((Data(open).buffer_d = open_dos_alloc(0x1000)) == 0) {
    CursorPrint(open, std::array<uint16_t, 1>{ Near(open, Data(open).alloc_err6) });
    Hang();
  }
  if ((Data(open).buffer_e = open_dos_alloc(0x1000)) == 0) {
    CursorPrint(open, std::array<uint16_t, 1>{ Near(open, Data(open).alloc_err7) });
    Hang();
  }
  if ((Data(open).buffer_f = open_dos_alloc(0x1000)) == 0) {
    CursorPrint(open, std::array<uint16_t, 1>{ Near(open, Data(open).alloc_err9) });
    Hang();
  }
  handle = opeu_open_file_or_die(open, Near(open, Data(open).name_mopengrp));
  ReadFar(open, static_cast<int16_t>(handle), { Data(open).buffer_h, 0, 0x6d40 });
  opeu_dos_close(static_cast<int16_t>(handle));
  koei_09b7(1, 0);
  opeu_vga_clear();
  opeu_ui_text(open, Near(open, Data(open).text_console));
  opeu_open_identity_palette(open);
  opeu_sound_initialize(open);
  opeu_mouse_reset(open);
}

void opeu_open_free_buffers(Open& open) {
  opeu_vga_clear();
  open_dos_free(Data(open).buffer_g);
  open_dos_free(Data(open).buffer_h);
  open_dos_free(Data(open).buffer_a);
  open_dos_free(Data(open).buffer_b);
  open_dos_free(Data(open).buffer_c);
  open_dos_free(Data(open).buffer_d);
  open_dos_free(Data(open).buffer_e);
  open_dos_free(Data(open).buffer_f);
}

/* Music driver (int 66h): AH=1 AL=5, then AH=2 AL=20. */
void opeu_open_shutdown(Open& open) {
  opeu_open_fade_in_wait(open);
  StartSong(5);
  StopMusic(20);
  opeu_vga_clear();
  opeu_open_fade_alt(open);
  while (!FileExists(open, Near(open, Data(open).name_scesel)));
  opeu_open_free_buffers(open);
}

void opeu_open_files_open(Open& open) {
  Data(open).picture_handle = static_cast<int16_t>(opeu_open_file_or_die(open, Near(open, Data(open).name_npk)));
  Data(open).logo_file      = static_cast<int16_t>(opeu_open_file_or_die(open, Near(open, Data(open).name_fnt)));
}

void opeu_open_files_close(Open& open) {
  opeu_dos_close(static_cast<int16_t>(Data(open).picture_handle));
  opeu_dos_close(static_cast<int16_t>(Data(open).logo_file));
}

void opeu_open_main(Open& open) {
  koei_install_dos_handlers();
  opeu_open_init(open);
  opeu_open_files_open(open);
  for (;;) {
    opeu_fm_wait_start(20000);
    opeu_open_scene_koei(open);
    if (opeu_open_scene_prologue(open) != 0) {
      Data(open).aborted = 1;
      break;
    }
    if (opeu_open_scene_color_cycle(open) != 0) {
      Data(open).aborted = 1;
      break;
    }
    if (opeu_open_scene_flicker(open) != 0) {
      Data(open).aborted = 1;
      break;
    }
    if (opeu_open_scene_panorama(open) != 0) {
      Data(open).aborted = 1;
      break;
    }
    if (opeu_open_scene_units(open) != 0) {
      Data(open).aborted = 1;
      break;
    }
    if (opeu_open_scene_bombing(open) != 0) {
      Data(open).aborted = 1;
      break;
    }
    if (opeu_open_scene_flak(open) != 0) {
      Data(open).aborted = 1;
      break;
    }
    if (opeu_open_scene_artillery(open) != 0) {
      Data(open).aborted = 1;
      break;
    }
    if (opeu_open_title(open) != 0) {
      Data(open).aborted = 0;
      break;
    }
  }
  if (Data(open).aborted != 0) opeu_open_title(open);
  opeu_open_files_close(open);
  opeu_open_shutdown(open);
  opeu_fm_noop();
}

}
