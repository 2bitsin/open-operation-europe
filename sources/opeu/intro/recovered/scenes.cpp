// The opening's scenes, from the KOEI logo to the title, and the start-up and shutdown around them.
//
// Timing: the effect countdown drives the uninterruptible Delay, which only waits when ticks != 0; the music timeline,
// counting down, is what WaitTicks (relative) and WaitForMark (absolute) poll together with the input (a key or a mouse
// button aborts, result 1). Every abortable scene returns 1 when aborted, 0 when it completed. Pictures come from
// OPENGRP.NPK by absolute file position; screen captures go to the six 64 KiB far buffers buffer_a..buffer_f;
// palettes are 48-byte rows of the palette table.
//
// The scenes: the logo and its captions; the prologue; the colour cycle and the descent; the flicker and the dissolve;
// the march and the panorama; the units, two vehicles driving in on 2-byte strips, then the eleven 160x192 frames of
// frame_y/frame_h/frame_off (frames 8-10 loop); the bombing, a bobbing bomber (bob_a, a rotating 8x64 part) fading in
// over 58 frames on the music timeline, then three explosions composed in far buffers (copy, 1-bit mask AND, sprite
// OR) and played with palette flashes; the flak, three flashes, then the bomber again (bob_b) among four bursts until
// the palette is black; the artillery, ground and gun captured in columns and panned in, then shells fired along the
// shell tables with a muzzle flash flicker (flash_on, flash_phase); the title, colour-cycled (the short form after an
// abort), then the wait for the music's end. StartUp and ShutDown allocate and free the far buffers, read
// MOPENGRP.DAT, and wait for SCESEL.DAT before leaving. RunOpening repeats the opening until a scene is aborted (then
// the short title) or the title is skipped.
//
// Near pointers are DGROUP offsets; all arithmetic wraps at 16 bits.
#include "recovered.hpp"

#include "load-image.hpp"
#include "repeat.hpp"
#include "segment-buffers.hpp"
#include "services.hpp"
#include "vgalib.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <ranges>

namespace opeu::intro::detail::scenes {
namespace {
// A picture of OPENGRP.NPK by its absolute file position.
auto PictureAt(Open& open, uint16_t x, uint16_t y, uint32_t position) -> void {
  DrawPicture(open, x, y, static_cast<int32_t>(position));
}

// Palette n of the palette table.
auto PaletteNumber(Open& open, std::size_t n) -> Palette {
  return Data(open).palettes[n];
}

// Whether the file named at a near pointer exists. The shutdown busy-waits for SCESEL.DAT with it; the host stays
// responsive while the file is missing (no effect on the result).
auto FileExists(Open const& open, uint16_t name) -> bool {
  auto const found{ opeu_dos_exists(reinterpret_cast<char const*>(&Byte(open, name))) }; // NOLINT(*-reinterpret-cast): the DOS name's characters
  if (found == 0) port_pump();
  return found != 0;
}
} // namespace

auto WaitForMark(uint16_t target) -> int;
auto ClearScreen() -> void;
auto FadeToPalette(Open& open, uint8_t slot, int mode) -> int;
auto LoadKoeiLogo(Open& open) -> void;
auto ShowKoeiLogo(Open& open) -> void;
auto ShowFirstCaption(Open& open) -> void;
auto ShowSecondCaption(Open& open) -> void;
auto PlayKoeiScene(Open& open) -> void;
auto CaptureProloguePictures(Open& open) -> int;
auto PlayPrologueScene(Open& open) -> int;
auto PlayDescent(Open& open) -> int;
auto PlayColorCycleScene(Open& open) -> int;
auto PlayFlickerScene(Open& open) -> int;
auto PlaySpriteMarch(Open& open) -> int;
auto ScrollPanorama(Open& open) -> int;
auto FlashPanorama(Open& open) -> int;
auto LeavePanorama(Open& open) -> int;
auto PlayPanoramaScene(Open& open) -> int;
auto DriveUnitsIn(Open& open) -> int;
auto AnimateUnits(Open& open) -> int;
auto PlayUnitsScene(Open& open) -> int;
auto FlyBomberIn(Open& open) -> int;
auto PlayExplosions(Open& open) -> int;
auto PlayBombingScene(Open& open) -> int;
auto PlayFlakFlashes(Open& open) -> int;
auto FlyBomberThroughFlak(Open& open) -> int;
auto PlayFlakScene(Open& open) -> int;
auto SetUpGun(Open& open) -> int;
auto PanToGun(Open& open) -> int;
auto ToggleMuzzleFlash(Open& open) -> void;
auto FireShells(Open& open, std::size_t first) -> int;
auto FireShellPairs(Open& open) -> int;
auto FireGun(Open& open) -> int;
auto PlayArtilleryScene(Open& open) -> int;
auto PlayTitle(Open& open) -> int;
auto StartUp(Open& open) -> void;
auto FreeBuffers(Open& open) -> void;
auto ShutDown(Open& open) -> void;
auto OpenFiles(Open& open) -> void;
auto CloseFiles(Open& open) -> void;

auto WaitForMark(uint16_t target) -> int {
  auto aborted{ false };
  RepeatWhile([&] { return !aborted && opeu_fm_busy() > target; }, [&] { aborted = PollInput() != 0; });
  return aborted ? 1 : 0;
}

auto ClearScreen() -> void {
  opeu_vga_clear();
}

auto FadeToPalette(Open& open, uint8_t slot, int mode) -> int {
  return FadePalette(open, PaletteNumber(open, slot), static_cast<uint16_t>(mode));
}

auto LoadKoeiLogo(Open& open) -> void {
  opeu_dos_seek(Data(open).logo_handle, 0L, 0);
  ReadFar(open, static_cast<int16_t>(Data(open).logo_handle), { Data(open).buffer_a, 0, 0xb60 });
  opeu_dos_seek(Data(open).logo_handle, 0x1860L, 0);
  ReadFar(open, static_cast<int16_t>(Data(open).logo_handle), { Data(open).buffer_a, 0xb60, 0x270 });
}

auto ShowKoeiLogo(Open& open) -> void {
  ClearScreen();
  FadeToPalette(open, 2, -1);
  DrawFar(open, { 0x1b, 0x60, 0x1a, 0x70 }, { Data(open).buffer_a, 0, 2 });
  Delay(60);
  DrawFar(open, { 0x1b, 0xe0, 0x1a, 0x18 }, { Data(open).buffer_a, 0xb60, 7 });
  Delay(60);
  WaitForMark(0xe98a);
  FadeOutUninterrupted(open);
}

auto ShowFirstCaption(Open& open) -> void {
  ClearScreen();
  FadeToPalette(open, 2, -1);
  PictureAt(open, 0x80, 0xbc, 0L);
  Delay(0x78);
  WaitForMark(0xe868);
  FadeOutUninterrupted(open);
}

auto ShowSecondCaption(Open& open) -> void {
  ClearScreen();
  FadeToPalette(open, 2, -1);
  PictureAt(open, 0xf2, 0x76, 0x7f0L);
  PictureAt(open, 0x80, 0xba, 0x42dL);
  Delay(0x78);
  WaitForMark(0xe50c);
  FadeOutUninterrupted(open);
}

auto PlayKoeiScene(Open& open) -> void {
  LoadKoeiLogo(open);
  ShowKoeiLogo(open);
  ShowFirstCaption(open);
  ShowSecondCaption(open);
}

auto CaptureProloguePictures(Open& open) -> int {
  ClearScreen();
  PictureAt(open, 0, 0, 0x2955fL);
  SaveScreen(open, { 0, 0, 0x50, 0x8c }, { Data(open).buffer_e, 0 });
  SaveScreen(open, { 0, 0x8c, 0x50, 0x8c }, { Data(open).buffer_f, 0 });
  ClearScreen();
  PictureAt(open, 0x58, 0, 0xa81L);
  if (FadeToPalette(open, 3, 2) != 0) return 1;
  SaveScreen(open, { 10, 0, 0x3c, 0xc8 }, { Data(open).buffer_b, 0 });
  SaveScreen(open, { 10, 0xc8, 0x3c, 0xc8 }, { Data(open).buffer_c, 0 });
  if (WaitTicks(0x105) != 0) return 1;
  if (WaitForMark(0xe25d) != 0) return 1;
  if (FadeOut(open) != 0) return 1;
  return 0;
}

auto PlayPrologueScene(Open& open) -> int {
  if (WaitForMark(0xe4bf) != 0) return 1;
  StartMusic(open, 0);
  SendMusicControl(open, 1);
  if (CaptureProloguePictures(open) != 0) return 1;
  ClearScreen();
  PictureAt(open, 0x90, 0x3c, 0x93f7L);
  if (FadeToPalette(open, 4, 2) != 0) return 1;
  SaveScreen(open, { 0x12, 0x3c, 0x30, 0x154 }, { Data(open).buffer_d, 0 });
  if (WaitTicks(0xdc) != 0) return 1;
  if (WaitForMark(0xdfa9) != 0) return 1;
  if (FadeOut(open) != 0) return 1;
  return 0;
}

auto PlayDescent(Open& open) -> int {
  uint16_t h     = 0;
  uint16_t off_a = 0;
  uint16_t off_b = 0;
  ClearScreen();
  if (FadeToPalette(open, 30, 2) != 0) return 1;
  h     = 0;
  off_a = 0x2580;
  off_b = 0x4a40;
  for (auto const i : std::views::iota(0, 25)) {
    BlitFar(open, { 0x22, 0, 12, h += 2 }, { Data(open).buffer_a, (off_a -= 0x60), 0 });
    if (WaitTicks(6) != 0) return 1;
    if (i > 21) StepPalette(open, PaletteNumber(open, 0));
    BlitFar(open, { 0x22, 0, 12, h += 2 }, { Data(open).buffer_a, (off_a -= 0x60), 0 });
    if (WaitTicks(6) != 0) return 1;
    if (i > 21) StepPalette(open, PaletteNumber(open, 0));
    off_a -= 0xc0;
    BlitFar(open, { 0x22, 0, 12, h += 2 }, { Data(open).buffer_a, (off_b -= 0x60), 0 });
    if (WaitTicks(6) != 0) return 1;
    if (i > 21) StepPalette(open, PaletteNumber(open, 0));
    BlitFar(open, { 0x22, 0, 12, h += 2 }, { Data(open).buffer_a, (off_b -= 0x60), 0 });
    if (WaitTicks(6) != 0) return 1;
    if (i > 21) StepPalette(open, PaletteNumber(open, 0));
    off_b -= 0xc0;
  }
  if (FadeOut(open) != 0) return 1;
  return 0;
}

auto PlayColorCycleScene(Open& open) -> int {
  ClearScreen();
  PictureAt(open, 0x58, 0, 0x121a0L);
  if (FadeToPalette(open, 5, 2) != 0) return 1;
  if (WaitTicks(0x103) != 0) return 1;
  if (WaitForMark(0xdced) != 0) return 1;
  SetCurrentPalette(open, PaletteNumber(open, 6));
  if (WaitTicks(5) != 0) return 1;
  SetCurrentPalette(open, PaletteNumber(open, 7));
  if (WaitTicks(5) != 0) return 1;
  SetCurrentPalette(open, PaletteNumber(open, 8));
  if (WaitTicks(5) != 0) return 1;
  SetCurrentPalette(open, PaletteNumber(open, 9));
  if (WaitTicks(5) != 0) return 1;
  SetCurrentPalette(open, PaletteNumber(open, 5));
  if (WaitTicks(0x14) != 0) return 1;
  if (FadeOut(open) != 0) return 1;
  ClearScreen();
  PictureAt(open, 0, 0, 0x28343L);
  PictureAt(open, 0x60, 0, 0x28c1bL);
  SaveScreen(open, { 0, 0, 12, 0xc8 }, { Data(open).buffer_a, 0 });
  SaveScreen(open, { 12, 0, 12, 0xc8 }, { Data(open).buffer_a, 0x2580 });
  ClearScreen();
  PictureAt(open, 0, 0, 0x27b7dL);
  for (auto const pal : std::views::iota(11u, 30u)) {
    SetCurrentPalette(open, PaletteNumber(open, pal));
    if (WaitTicks(5) != 0) return 1;
  }
  if (FadeToPalette(open, 0, 1) != 0) return 1;
  if (PlayDescent(open) != 0) return 1;
  return 0;
}

auto PlayFlickerScene(Open& open) -> int {
  ClearScreen();
  BlitFar(open, { 0, 0x3c, 0x50, 0x8c }, { Data(open).buffer_e, 0, 0 });
  BlitFar(open, { 0, 0xc8, 0x50, 0x8c }, { Data(open).buffer_f, 0, 0 });
  if (FadeToPalette(open, 31, 2) != 0) return 1;
  for ([[maybe_unused]] auto const flicker : std::views::iota(0, 2)) {
    SetCurrentPalette(open, PaletteNumber(open, 32));
    if (WaitTicks(6) != 0) return 1;
    SetCurrentPalette(open, PaletteNumber(open, 31));
    if (WaitTicks(6) != 0) return 1;
  }
  for ([[maybe_unused]] auto const flicker : std::views::iota(0, 10)) {
    SetCurrentPalette(open, PaletteNumber(open, 32));
    if (WaitTicks(5) != 0) return 1;
    SetCurrentPalette(open, PaletteNumber(open, 31));
    if (WaitTicks(8) != 0) return 1;
  }
  if (WaitTicks(0x78) != 0) return 1;
  if (FadeOut(open) != 0) return 1;
  ClearScreen();
  PictureAt(open, 0, 0, 0x399b9L);
  SaveScreen(open, { 0, 0, 0x12, 0x78 }, { Data(open).buffer_a, 0 });
  SaveScreen(open, { 0x12, 0, 0x12, 0x78 }, { Data(open).buffer_a, 0x21c0 });
  SaveScreen(open, { 0x24, 0, 0x12, 0x78 }, { Data(open).buffer_a, 0x4380 });
  SaveScreen(open, { 0, 0x78, 0x12, 0x78 }, { Data(open).buffer_a, 0x6540 });
  SaveScreen(open, { 0x12, 0x78, 0x12, 0x78 }, { Data(open).buffer_a, 0x8700 });
  SaveScreen(open, { 0x24, 0x78, 0x12, 0x78 }, { Data(open).buffer_a, 0xa8c0 });
  ClearScreen();
  BlitFar(open, { 0x12, 0x3c, 0x30, 0x154 }, { Data(open).buffer_d, 0, 0 });
  if (WaitTicks(0x3c) != 0) return 1;
  PictureAt(open, 0xc, 0x3c, 0x37110L);
  SaveScreen(open, { 0, 0x3c, 0x26, 0x120 }, { Data(open).buffer_e, 0 });
  ClearScreen();
  BlitFar(open, { 0x12, 0x3c, 0x30, 0x154 }, { Data(open).buffer_d, 0, 0 });
  if (FadeToPalette(open, 4, 2) != 0) return 1;
  if (WaitForMark(0xd56a) != 0) return 1;
  Dissolve(open, { 0, 0x3c }, { Data(open).buffer_e, 0, 0x0001 });
  Dissolve(open, { 0, 0x3c }, { Data(open).buffer_e, 0, 0x0101 });
  Dissolve(open, { 0, 0x3c }, { Data(open).buffer_e, 0, 0x1111 });
  Dissolve(open, { 0, 0x3c }, { Data(open).buffer_e, 0, 0x5555 });
  Dissolve(open, { 0, 0x3c }, { Data(open).buffer_e, 0, 0x7777 });
  Dissolve(open, { 0, 0x3c }, { Data(open).buffer_e, 0, 0x7f7f });
  Dissolve(open, { 0, 0x3c }, { Data(open).buffer_e, 0, 0xffff });
  if (WaitTicks(0xc8) != 0) return 1;
  if (WaitForMark(0xd3a2) != 0) return 1;
  if (FadeOut(open) != 0) return 1;
  ClearScreen();
  PictureAt(open, 0, 0, 0x3e98bL);
  for (auto const strip : std::views::iota(0, 79)) { // the columns right to left, 0x2c0 bytes each
    SaveScreen(open, { 78 - strip, 0, 1, 0xb0 }, { Data(open).buffer_d, static_cast<NearWord>(strip * 0x2c0) });
  }
  return 0;
}

auto PlaySpriteMarch(Open& open) -> int {
  uint16_t d = 0x40; // the ticks per frame: halved plus one after each pass
  ClearScreen();
  BlitFar(open, { 10, 0, 0x3c, 0xc8 }, { Data(open).buffer_b, 0, 0 });
  BlitFar(open, { 10, 0xc8, 0x3c, 0xc8 }, { Data(open).buffer_c, 0, 0 });
  if (FadeToPalette(open, 3, 2) != 0) return 1;
  if (WaitForMark(0xd2ed) != 0) return 1;
  for (auto const pass : std::views::iota(0)) {
    for (auto const frame : std::views::iota(0, 3)) {
      BlitFar(open, { 10, 0x118, 0x12, 0x78 }, { Data(open).buffer_a, static_cast<NearWord>(frame * 0x21c0), 0 });
      BlitFar(open, { 0x34, 0x118, 0x12, 0x78 }, { Data(open).buffer_a, static_cast<NearWord>(0x6540 + (frame * 0x21c0)), 0 });
      if (WaitTicks(d) != 0) return 1;
    }
    if (pass > 40 && (StepPalette(open, PaletteNumber(open, 0)) == 0)) {
      if (FadeOut(open) != 0) return 1;
      break;
    }
    d = static_cast<uint16_t>((d >> 1) + 1);
  }
  return 0;
}

auto ScrollPanorama(Open& open) -> int {
  ClearScreen();
  PictureAt(open, 0, 0, 0x40bd5L);
  SaveScreen(open, { 0, 0, 0x20, 0xb0 }, { Data(open).buffer_a, 0 });
  if (WaitTicks(1) != 0) return 1;
  PictureAt(open, 0, 0, 0x421d5L);
  SaveScreen(open, { 0, 0, 0x20, 0xb0 }, { Data(open).buffer_b, 0 });
  if (WaitTicks(1) != 0) return 1;
  PictureAt(open, 0, 0, 0x43202L);
  SaveScreen(open, { 0, 0, 0x20, 0xb0 }, { Data(open).buffer_c, 0 });
  ClearScreen();
  if (FadeToPalette(open, 33, 0) != 0) return 1;
  for (auto const strip : std::views::iota(0, 0x4f)) {
    opeu_screen_copy(0x19, 0x70, 0x1d, 0xb0, 0x1a, 0x70);
    BlitFar(open, { 0x19, 0x70, 1, 0xb0 }, { Data(open).buffer_d, static_cast<NearWord>(strip * 0x2c0), 0 });
    if (WaitTicks(5) != 0) return 1;
  }
  SaveScreen(open, { 0x19, 0x70, 0x20, 0xb0 }, { Data(open).buffer_d, 0 });
  if (WaitForMark(0xcc15) != 0) return 1;
  return 0;
}

auto FlashPanorama(Open& open) -> int {
  for (auto const i : std::views::iota(0, 2)) {
    SetCurrentPalette(open, PaletteNumber(open, 34));
    SetCurrentPalette(open, PaletteNumber(open, 35));
    if (WaitTicks(2) != 0) return 1;
    BlitFar(open, { 0x18, 0x70, 0x20, 0xb0 }, { Data(open).buffer_a, 0, 0 });
    SetCurrentPalette(open, PaletteNumber(open, 33));
    if (WaitTicks(10) != 0) return 1;
    BlitFar(open, { 0x18, 0x70, 0x20, 0xb0 }, { Data(open).buffer_b, 0, 0 });
    if (WaitTicks(10) != 0) return 1;
    BlitFar(open, { 0x18, 0x70, 0x20, 0xb0 }, { Data(open).buffer_c, 0, 0 });
    if (WaitTicks(10) != 0) return 1;
    BlitFar(open, { 0x18, 0x70, 0x20, 0xb0 }, { Data(open).buffer_d, 0, 0 });
    if (i == 0 && (WaitTicks(100) != 0)) return 1;
  }
  if (WaitTicks(0x1e) != 0) return 1;
  if (WaitForMark(0xc9f8) != 0) return 1;
  return 0;
}

auto LeavePanorama(Open& open) -> int {
  opeu_ui_box(0x38, 0x70, 1, 0xb0, 0);
  for ([[maybe_unused]] auto const step : std::views::iota(0, 15)) {
    opeu_screen_copy(0x17, 0x70, 0x1f, 0xb0, 0x18, 0x70);
    if (WaitTicks(5) != 0) return 1;
  }
  if (WaitForMark(0xc99e) != 0) return 1;
  if (FadeOut(open) != 0) return 1;
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

auto PlayPanoramaScene(Open& open) -> int {
  if (PlaySpriteMarch(open) != 0) return 1;
  if (ScrollPanorama(open) != 0) return 1;
  if (FlashPanorama(open) != 0) return 1;
  if (LeavePanorama(open) != 0) return 1;
  return 0;
}
auto DriveUnitsIn(Open& open) -> int {
  uint16_t first_strip  = 0;
  uint16_t second_strip = 0;
  ClearScreen();
  BlitFar(open, { 2, 0, 0x14, 0x60 }, { Data(open).buffer_a, 0, 0 });
  first_strip = 0;
  for (auto const strip : std::views::iota(0, 11)) { // the odd columns 21..1
    SaveScreen(open, { 0x15 - (2 * strip), 0, 2, 0x60 }, { Data(open).buffer_c, first_strip });
    first_strip += 0x300;
  }
  for (auto const strip : std::views::iota(0, 11)) { // the even columns 20..0
    SaveScreen(open, { 0x14 - (2 * strip), 0, 2, 0x60 }, { Data(open).buffer_c, first_strip });
    first_strip += 0x300;
  }
  ClearScreen();
  SetCurrentPalette(open, PaletteNumber(open, 36));
  second_strip = first_strip = 0;
  for (auto const x : std::views::iota(0, 31)) {
    opeu_screen_copy(0, 0xc4, 0x3c, 0x60, 2, 0xc4);
    if (x < 11) {
      BlitFar(open, { 0, 0xc4, 2, 0x60 }, { Data(open).buffer_c, first_strip, 0 });
      first_strip += 0x300;
    }
    if (x > 12 && x < 24) {
      BlitFar(open, { 0, 0xc4, 2, 0x60 }, { Data(open).buffer_c, second_strip, 0 });
      second_strip += 0x300;
    }
    if (WaitTicks(6) != 0) return 1;
  }
  if (WaitTicks(10) != 0) return 1;
  return 0;
}

auto AnimateUnits(Open& open) -> int {
  std::size_t right_frame = 0;
  std::size_t left_frame  = 0;
  for (auto const tick : std::views::iota(0, 40)) {
    if (tick == 0) {
      SetCurrentPalette(open, PaletteNumber(open, 34));
      SetCurrentPalette(open, PaletteNumber(open, 35));
      if (WaitTicks(2) != 0) return 1;
      SetCurrentPalette(open, PaletteNumber(open, 36));
    }
    opeu_ui_box(0x2a, 0x64, 0x14, 0x80, 0);
    BlitFar(open, { 0x2a, Data(open).frame_y[right_frame], 0x14, Data(open).frame_h[right_frame] },
            { right_frame < 5 ? Data(open).buffer_a : Data(open).buffer_b, Data(open).frame_off[right_frame], 0 });
    right_frame++;
    if (right_frame == 11) right_frame = 8;
    opeu_ui_box(0x10, 0x64, 0x14, 0x80, 0);
    BlitFar(open, { 0x10, Data(open).frame_y[left_frame], 0x14, Data(open).frame_h[left_frame] }, { left_frame < 5 ? Data(open).buffer_a : Data(open).buffer_b, Data(open).frame_off[left_frame], 0 });
    if (tick > 2) {
      left_frame++;
      if (left_frame == 11) left_frame = 8;
    }
    if (WaitTicks(10) != 0) return 1;
    if (tick > 30) StepPalette(open, PaletteNumber(open, 0));
  }
  if (FadeOut(open) != 0) return 1;
  if (WaitForMark(0xc5e2) != 0) return 1;
  return 0;
}

auto PlayUnitsScene(Open& open) -> int {
  if (DriveUnitsIn(open) != 0) return 1;
  if (AnimateUnits(open) != 0) return 1;
  return 0;
}

auto FlyBomberIn(Open& open) -> int {
  uint16_t    composite   = 0;
  uint16_t    deadline    = 0;
  std::size_t bob         = 0;
  uint16_t    y           = 0;
  int         faded_in    = 0;
  uint16_t    part_offset = 0;
  uint16_t    next_y      = 0;
  ClearScreen();
  PictureAt(open, 0, 0, 0x46848L);
  SaveScreen(open, { 0, 0, 0x50, 0xa0 }, { Data(open).buffer_c, 0 });
  ClearScreen();
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
  ClearScreen();
  PictureAt(open, 0, 0, 0x47428L);
  SaveScreen(open, { 0, 0, 0x10, 0x90 }, { Data(open).buffer_a, 0x2800 });
  SaveScreen(open, { 0x10, 0, 0x10, 0x90 }, { Data(open).buffer_a, 0x7000 });
  SaveScreen(open, { 0x20, 0, 0x10, 0x90 }, { Data(open).buffer_a, 0x4c00 });
  SaveScreen(open, { 0x30, 0, 0x10, 0x90 }, { Data(open).buffer_a, 0x9400 });
  ClearScreen();
  BlitFar(open, { 0, 0x50, 0x50, 0xa0 }, { Data(open).buffer_c, 0, 0 });
  BlitFar(open, { 0x13, 0x50, 0x10, 0x90 }, { Data(open).buffer_a, 0x2800, 0 });
  BlitFar(open, { 0x32, 0x50, 0x10, 0x90 }, { Data(open).buffer_a, 0x4c00, 0 });
  BlitFar(open, { 0, 0xf0, 0x50, 0xa0 }, { Data(open).buffer_c, 0, 0 });
  BlitFar(open, { 0x13, 0xf0, 0x10, 0x90 }, { Data(open).buffer_a, 0x7000, 0 });
  BlitFar(open, { 0x32, 0xf0, 0x10, 0x90 }, { Data(open).buffer_a, 0x9400, 0 });
  SaveScreen(open, { 0, 0x50, 0x50, 0xa0 }, { Data(open).buffer_c, 0 });
  SaveScreen(open, { 0, 0xf0, 0x50, 0xa0 }, { Data(open).buffer_d, 0 });
  ClearScreen();
  BlitFar(open, { 0, 0x50, 0x50, 0xa0 }, { Data(open).buffer_d, 0, 0 });
  next_y = y = Data(open).bob_a[0];
  for ([[maybe_unused]] auto const frame : std::views::iota(0, 0x3a)) {
    deadline  = opeu_fm_busy();
    deadline -= 12;
    y         = Data(open).bob_a[bob];
    BlitFar(open, { 0, y, 0x50, 0xa0 }, { (composite != 0u) ? Data(open).buffer_d : Data(open).buffer_c, 0, 0 });
    BlitFar(open, { 0x23, y + 0x60, 8, 0x40 }, { Data(open).buffer_a, part_offset, 0 });
    composite ^= 1;
    bob++;
    if (bob > 19) bob = 0;
    next_y = Data(open).bob_a[bob];
    if (next_y < y) opeu_ui_box(0, next_y + 0xa0, 0x50, y - next_y, 0);
    part_offset += 0x800;
    if (part_offset >= 0x2800) part_offset = 0;
    if ((faded_in == 0) && (StepPalette(open, PaletteNumber(open, 0x25)) == 0)) faded_in = 1;
    if (WaitForMark(deadline) != 0) return 1;
  }
  if (FadeOut(open) != 0) return 1;
  return 0;
}

// The opening's recovered animation script; it goes
// when the opening becomes a scene (M6).
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
auto PlayExplosions(Open& open) -> int {
  ClearScreen();
  SetCurrentPalette(open, PaletteNumber(open, 0x26));
  BlitFar(open, { 0xf, 0x28, 0x32, 0x140 }, { Data(open).buffer_b, 0, 0 });
  if (WaitForMark(0xbd4b) != 0) return 1;
  if (WaitTicks(50) != 0) return 1;
  for (auto const blast : std::views::iota(0uz, 3uz)) {
    SaveScreen(open, { Data(open).blast_x[blast], Data(open).blast_y[blast], 0x1a, 0xb0 }, { Data(open).buffer_a, 0xb800 });
    for (auto const frame : std::views::iota(0uz, 3uz)) {
      auto const frame_offset{ static_cast<NearWord>(frame * 0x4780) };
      CopyFar(open, { Data(open).buffer_a, 0xb800 }, { Data(open).buffer_b, frame_offset, 0x4780 });
      AndMask(open, { Data(open).buffer_b, frame_offset, 0x1a, 0xb0 }, { Data(open).sprite_sheet, Data(open).blast_masks[frame] });
      OrSprite(open, { Data(open).buffer_b, frame_offset, 0x1a, 0xb0 }, { Data(open).buffer_e, Data(open).blast_sprites[frame] });
    }
    for (auto const frame : std::views::iota(3uz, 6uz)) {
      auto const frame_offset{ static_cast<NearWord>((frame - 3) * 0x4780) };
      CopyFar(open, { Data(open).buffer_a, 0xb800 }, { Data(open).buffer_c, frame_offset, 0x4780 });
      AndMask(open, { Data(open).buffer_c, frame_offset, 0x1a, 0xb0 }, { Data(open).sprite_sheet, Data(open).blast_masks[frame] });
      OrSprite(open, { Data(open).buffer_c, frame_offset, 0x1a, 0xb0 }, { Data(open).buffer_f, Data(open).blast_sprites[frame] });
    }
    for (auto const frame : std::views::iota(0uz, 6uz)) {
      auto const frame_offset{ static_cast<NearWord>((frame % 3) * 0x4780) };
      BlitFar(open, { Data(open).blast_x[blast], Data(open).blast_y[blast], 0x1a, 0xb0 }, { frame < 3 ? Data(open).buffer_b : Data(open).buffer_c, frame_offset, 4 });
      for ([[maybe_unused]] auto const step : std::views::iota(0, static_cast<int>(Data(open).blast_steps[frame]))) StepPalette(open, PaletteNumber(open, Data(open).blast_slots[blast]));
      if (WaitTicks(2) != 0) return 1;
    }
    BlitFar(open, { Data(open).blast_x[blast], Data(open).blast_y[blast], 0x1a, 0xb0 }, { Data(open).buffer_a, 0xb800, 0 });
    if (FadeToPalette(open, 0x26, 1) != 0) return 1;
    BlitFar(open, { Data(open).blast_x[blast], Data(open).blast_y[blast], 0x1a, 0xb0 }, { Data(open).buffer_a, 0xb800, 0 });
    if (WaitTicks(10) != 0) return 1;
  }
  if (WaitTicks(90) != 0) return 1;
  if (WaitForMark(0xbada) != 0) return 1;
  if (FadeOut(open) != 0) return 1;
  return 0;
}

auto PlayBombingScene(Open& open) -> int {
  if (FlyBomberIn(open) != 0) return 1;
  if (PlayExplosions(open) != 0) return 1;
  return 0;
}

auto PlayFlakFlashes(Open& open) -> int {
  ClearScreen();
  PictureAt(open, 0x4c, 0xc, 0x54d6dL);
  if (FadeToPalette(open, 0x2a, 2) != 0) return 1;
  if (WaitTicks(100) != 0) return 1;
  if (WaitForMark(0xb8d3) != 0) return 1;
  for (auto const i : std::views::iota(0uz, 3uz)) {
    SetCurrentPalette(open, PaletteNumber(open, Data(open).flash_slots[i]));
    RepeatUntil([&] { return StepPalette(open, PaletteNumber(open, 0x2a)) == 0; });
    if (WaitTicks(60) != 0) return 1;
  }
  if (WaitTicks(30) != 0) return 1;
  if (WaitForMark(0xb6f8) != 0) return 1;
  if (FadeOut(open) != 0) return 1;
  ClearScreen();
  PictureAt(open, 0, 0xa, 0x5b770L);
  SaveScreen(open, { 0, 0, 6, 0x3c }, { Data(open).buffer_b, 0 });
  SaveScreen(open, { 6, 0, 6, 0x3c }, { Data(open).buffer_b, 0x5a0 });
  SaveScreen(open, { 0xc, 0, 6, 0x3c }, { Data(open).buffer_b, 0xb40 });
  SaveScreen(open, { 0x12, 0, 6, 0x3c }, { Data(open).buffer_b, 0x10e0 });
  ClearScreen();
  PictureAt(open, 0, 0, 0x5b9e3L);
  SaveScreen(open, { 0, 0, 0x20, 0xe0 }, { Data(open).buffer_e, 0 });
  PictureAt(open, 0, 0, 0x5d486L);
  SaveScreen(open, { 0, 0, 0x20, 0xe0 }, { Data(open).buffer_e, 0x7000 });
  if (WaitTicks(15) != 0) return 1;
  if (WaitForMark(0xb6f8) != 0) return 1;
  if (FadeOut(open) != 0) return 1;
  return 0;
}

auto FlyBomberThroughFlak(Open& open) -> int {
  uint16_t    composite    = 0;
  int         burst_phase  = 0;
  uint16_t    deadline     = 0;
  std::size_t bob          = 0;
  uint16_t    y            = 0;
  uint16_t    part_offset  = 0;
  uint16_t    next_y       = 0;
  uint16_t    burst_offset = 0;
  ClearScreen();
  BlitFar(open, { 0, 0x50, 0x50, 0xa0 }, { Data(open).buffer_d, 0, 0 });
  BlitFar(open, { 0x13, 0x50, 0x10, 0x90 }, { Data(open).buffer_a, 0x2800, 0 });
  BlitFar(open, { 0x32, 0x50, 0x10, 0x90 }, { Data(open).buffer_a, 0x4c00, 0 });
  SaveScreen(open, { 0, 0x50, 0x50, 0xa0 }, { Data(open).buffer_c, 0 });
  ClearScreen();
  SetCurrentPalette(open, PaletteNumber(open, 0x2e));
  BlitFar(open, { 0, 0x50, 0x50, 0xa0 }, { Data(open).buffer_d, 0, 0 });
  next_y = y = Data(open).bob_b[0];
  for (auto const frame : std::views::iota(0)) {
    deadline  = opeu_fm_busy();
    deadline -= 12;
    y         = Data(open).bob_b[bob];
    BlitFar(open, { 0, y, 0x50, 0xa0 }, { (composite != 0u) ? Data(open).buffer_d : Data(open).buffer_c, 0, 0 });
    BlitFar(open, { 0x23, y + 0x60, 8, 0x40 }, { Data(open).buffer_a, part_offset, 0 });
    composite ^= 1;
    bob++;
    if (bob > 19) bob = 0;
    next_y = Data(open).bob_b[bob];
    if (next_y < y) opeu_ui_box(0, next_y + 0xa0, 0x50, y - next_y, 0);
    burst_phase = frame % 10;
    if (burst_phase > 4 && burst_phase < 9) {
      BlitFar(open, { Data(open).flak_x[static_cast<std::size_t>(frame / 10 % 4)] - burst_phase + 5, Data(open).flak_y[static_cast<std::size_t>(frame / 10 % 4)], 6, 0x3c },
              { Data(open).buffer_b, burst_offset, 0 });
      burst_offset += 0x5a0;
      if (burst_offset >= 0x1680) burst_offset = 0;
    }
    part_offset += 0x800;
    if (part_offset >= 0x2800) part_offset = 0;
    if ((frame >= 30) && (StepPalette(open, PaletteNumber(open, 0)) == 0)) break;
    if (WaitForMark(deadline) != 0) return 1;
  }
  if (FadeOut(open) != 0) return 1;
  return 0;
}

auto PlayFlakScene(Open& open) -> int {
  if (PlayFlakFlashes(open) != 0) return 1;
  if (FlyBomberThroughFlak(open) != 0) return 1;
  return 0;
}

auto SetUpGun(Open& open) -> int {
  int crew_frame = 0;
  ClearScreen();
  PictureAt(open, 0, 0, 0x5eddeL);
  SaveScreen(open, { 0, 0, 6, 0x30 }, { Data(open).buffer_a, 0 });
  SaveScreen(open, { 6, 0, 6, 0x30 }, { Data(open).buffer_a, 0x480 });
  SaveScreen(open, { 0xc, 0, 6, 0x30 }, { Data(open).buffer_a, 0x900 });
  SaveScreen(open, { 0x12, 0, 6, 0x30 }, { Data(open).buffer_a, 0xd80 });
  ClearScreen();
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
  for (auto const strip : std::views::iota(0, 0x36)) { // the columns right to left
    SaveScreen(open, { 0x35 - strip, 0x108, 1, 0x30 }, { Data(open).buffer_b, static_cast<NearWord>(strip * 0xc0) });
  }
  ClearScreen();
  PictureAt(open, 0, 0x88, 0x5f135L);
  if (WaitTicks(1) != 0) return 1;
  BlitFar(open, { 0x28, 0x108, 6, 0x30 }, { Data(open).buffer_a, 0x900, 0 });
  for (auto const strip : std::views::iota(0, 0x28)) { // the columns right to left
    SaveScreen(open, { 0x27 - strip, 0x88, 1, 0xe0 }, { Data(open).buffer_c, static_cast<NearWord>(strip * 0x380) });
  }
  ClearScreen();
  if (FadeToPalette(open, 0x2f, 2) != 0) return 1;
  BlitFar(open, { 0x18, 0x58, 0x20, 0xe0 }, { Data(open).buffer_e, 0, 0 });
  BlitFar(open, { 0x12, 0x108, 6, 0x30 }, { Data(open).buffer_a, 0, 0 });
  BlitFar(open, { 0xc, 0x108, 6, 0x30 }, { Data(open).buffer_a, 0x900, 0 });
  BlitFar(open, { 6, 0x108, 6, 0x30 }, { Data(open).buffer_a, 0x480, 0 });
  BlitFar(open, { 0, 0x108, 6, 0x30 }, { Data(open).buffer_a, 0, 0 });
  SaveScreen(open, { 1, 0x58, 0x36, 0xe0 }, { Data(open).buffer_d, 0 });
  if (WaitTicks(100) != 0) return 1;
  if (WaitForMark(0xb121) != 0) return 1;
  for (auto const salvo : std::views::iota(0uz, 3uz)) {
    for ([[maybe_unused]] auto const flip : std::views::iota(0, static_cast<int>(Data(open).gun_flips[salvo]))) {
      BlitFar(open, { 0x18, 0x58, 0x20, 0xe0 }, { Data(open).buffer_e, static_cast<NearWord>((crew_frame != 0) ? 0 : 0x7000), 0 });
      if (WaitTicks((crew_frame != 0) ? 2 : 4) != 0) return 1;
      crew_frame ^= 1;
    }
    if (WaitTicks(100) != 0) return 1;
  }
  return 0;
}

auto PanToGun(Open& open) -> int {
  for (auto const strip : std::views::iota(0, 0x36)) {
    opeu_screen_copy(0, 0x58, 0x36, 0xe0, 1, 0x58);
    BlitFar(open, { 0, 0x108, 1, 0x30 }, { Data(open).buffer_b, static_cast<NearWord>(strip * 0xc0), 0 });
    if (PollInput() != 0) return 1;
  }
  for (auto const strip : std::views::iota(0, 0x28)) {
    opeu_screen_copy(0, 0x58, 0x36, 0x110, 1, 0x58);
    BlitFar(open, { 0, 0x88, 1, 0xe0 }, { Data(open).buffer_c, static_cast<NearWord>(strip * 0x380), 0 });
    if (PollInput() != 0) return 1;
  }
  if (WaitTicks(15) != 0) return 1;
  if (WaitForMark(0xac2d) != 0) return 1;
  return 0;
}

auto ToggleMuzzleFlash(Open& open) -> void {
  if (Data(open).flash_on != 0) {
    if (Data(open).flash_phase != 0)
      SetCurrentPalette(open, PaletteNumber(open, 0x2f));
    else
      SetCurrentPalette(open, PaletteNumber(open, 0x30));
    Data(open).flash_phase ^= 1;
  }
}

auto FireShells(Open& open, std::size_t first) -> int {
  for (auto const i : std::views::iota(first, first + 3)) {
    DrawFar(open, { Data(open).shell_x[i], Data(open).shell_y[i], Data(open).shell_w[i], Data(open).shell_h[i] }, { Data(open).sprite_sheet, Data(open).shell_sprite[i], 0x407 });
    if (WaitTicks(1) != 0) return 1;
    BlitFar(open, { Data(open).shell_x[i], Data(open).shell_y[i], Data(open).shell_ws[i], Data(open).shell_h[i] }, { Data(open).buffer_a, Data(open).shell_save[i], 0 });
    ToggleMuzzleFlash(open);
  }
  return 0;
}

auto FireShellPairs(Open& open) -> int {
  for (auto const i : std::views::iota(0uz, 3uz)) {
    auto const j{ i + 3 };
    DrawFar(open, { Data(open).shell_x[i], Data(open).shell_y[i], Data(open).shell_w[i], Data(open).shell_h[i] }, { Data(open).sprite_sheet, Data(open).shell_sprite[i], 0x407 });
    DrawFar(open, { Data(open).shell_x[j], Data(open).shell_y[j], Data(open).shell_w[j], Data(open).shell_h[j] }, { Data(open).sprite_sheet, Data(open).shell_sprite[j], 0x407 });
    if (WaitTicks(1) != 0) return 1;
    BlitFar(open, { Data(open).shell_x[i], Data(open).shell_y[i], Data(open).shell_ws[i], Data(open).shell_h[i] }, { Data(open).buffer_a, Data(open).shell_save[i], 0 });
    BlitFar(open, { Data(open).shell_x[j], Data(open).shell_y[j], Data(open).shell_ws[j], Data(open).shell_h[j] }, { Data(open).buffer_a, Data(open).shell_save[j], 0 });
    ToggleMuzzleFlash(open);
  }
  return 0;
}

// The opening's recovered animation script; it goes
// when the opening becomes a scene (M6).
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
auto FireGun(Open& open) -> int {
  int pair_salvos = 0;
  int stage       = 0;
  for (auto const frame : std::views::iota(0uz, 6uz))
    SaveScreen(open, { Data(open).shell_x[frame], Data(open).shell_y[frame], Data(open).shell_ws[frame], Data(open).shell_h[frame] }, { Data(open).buffer_a, Data(open).shell_save[frame] });
  for (auto const frame : std::views::iota(0, 50)) {
    if (frame == 7) {
      pair_salvos = 3;
      stage       = 1;
    }
    if (frame == 15) {
      pair_salvos = 10;
      stage       = 1;
    }
    if (frame == 35) {
      pair_salvos = 5;
      stage       = 1;
    }
    if (frame == 3 || frame == 12 || frame == 20 || frame == 23 || frame == 40 || frame == 49) Data(open).flash_on ^= 1;
    if (stage == 1) {
      FireShells(open, 0);
      stage = 2;
    } else if (stage == 2) {
      FireShellPairs(open);
      if (--pair_salvos == 0) stage = 3;
    } else if (stage == 3) {
      FireShells(open, 3);
      stage = 0;
    }
    if (Data(open).flash_on == 0)
      SetCurrentPalette(open, PaletteNumber(open, 0x2f));
    else
      ToggleMuzzleFlash(open);
    if (WaitTicks(1) != 0) return 1;
  }
  if (WaitTicks(0x50) != 0) return 1;
  if (WaitForMark(0xa8e2) != 0) return 1;
  if (FadeOut(open) != 0) return 1;
  return 0;
}

auto PlayArtilleryScene(Open& open) -> int {
  if (SetUpGun(open) != 0) return 1;
  if (PanToGun(open) != 0) return 1;
  if (FireGun(open) != 0) return 1;
  return 0;
}

auto PlayTitle(Open& open) -> int {
  int start = 0;
  ClearScreen();
  start = (Data(open).aborted != 0) ? 0x65 : 0x31;
  FadeOutUninterrupted(open);
  FadeToPalette(open, 0, 2);
  FadeOut(open);
  if (Data(open).aborted != 0) {
    FadeToPalette(open, 0x6f, 2);
    Delay(100);
  }
  PictureAt(open, 0x37, 0x3c, 0x5ff59L);
  for (auto const i : std::views::iota(start, 0x70)) {
    FadeToPalette(open, static_cast<uint8_t>(i), 2);
    if (Data(open).aborted != 0)
      Delay(15);
    else
      Delay(2);
  }
  Delay(60);
  PictureAt(open, 0x110, 0x148, 0x63a2cL);
  if (WaitTicks(0x103) != 0) return 1;
  if (WaitForMark(0xdced) != 0) return 1;
  SendMusicControl(open, 0);
  auto aborted{ false };
  RepeatWhile([&] { return !aborted && IsMusicPlaying(open) != 0; }, [&] { aborted = PollInput() != 0; });
  if (aborted) return 1;
  if (FadeOut(open) != 0) return 1;
  ClearScreen();
  return 0;
}

auto StartUp(Open& open) -> void {
  int handle = 0;
  if ((Data(open).stream_buffer = open.Memory().Allocate(0x501)) == 0) {
    CursorPrint(open, std::array<uint16_t, 1>{ Near(open, Data(open).alloc_err1) });
    Hang();
  }
  if ((Data(open).sprite_sheet = open.Memory().Allocate(0x6d5)) == 0) {
    CursorPrint(open, std::array<uint16_t, 1>{ Near(open, Data(open).alloc_err2) });
    Hang();
  }
  if ((Data(open).buffer_a = open.Memory().Allocate(0x1000)) == 0) {
    CursorPrint(open, std::array<uint16_t, 1>{ Near(open, Data(open).alloc_err3) });
    Hang();
  }
  if ((Data(open).buffer_b = open.Memory().Allocate(0x1000)) == 0) {
    CursorPrint(open, std::array<uint16_t, 1>{ Near(open, Data(open).alloc_err4) });
    Hang();
  }
  if ((Data(open).buffer_c = open.Memory().Allocate(0x1000)) == 0) {
    CursorPrint(open, std::array<uint16_t, 1>{ Near(open, Data(open).alloc_err5) });
    Hang();
  }
  if ((Data(open).buffer_d = open.Memory().Allocate(0x1000)) == 0) {
    CursorPrint(open, std::array<uint16_t, 1>{ Near(open, Data(open).alloc_err6) });
    Hang();
  }
  if ((Data(open).buffer_e = open.Memory().Allocate(0x1000)) == 0) {
    CursorPrint(open, std::array<uint16_t, 1>{ Near(open, Data(open).alloc_err7) });
    Hang();
  }
  if ((Data(open).buffer_f = open.Memory().Allocate(0x1000)) == 0) {
    CursorPrint(open, std::array<uint16_t, 1>{ Near(open, Data(open).alloc_err9) });
    Hang();
  }
  handle = OpenFileOrStop(open, Near(open, Data(open).sprite_file_name));
  ReadFar(open, static_cast<int16_t>(handle), { Data(open).sprite_sheet, 0, 0x6d40 });
  opeu_dos_close(static_cast<int16_t>(handle));
  opeu_vga_clear();
  PutString(open, Near(open, Data(open).console_reset));
  SetIdentityPalette(open);
  InitializeSound(open);
  ResetMouse(open);
}

auto FreeBuffers(Open& open) -> void {
  opeu_vga_clear();
  open.Memory().Free(Data(open).stream_buffer);
  open.Memory().Free(Data(open).sprite_sheet);
  open.Memory().Free(Data(open).buffer_a);
  open.Memory().Free(Data(open).buffer_b);
  open.Memory().Free(Data(open).buffer_c);
  open.Memory().Free(Data(open).buffer_d);
  open.Memory().Free(Data(open).buffer_e);
  open.Memory().Free(Data(open).buffer_f);
}

// The closing song (song 5) started and at once faded out, the closing fade, then the wait for the scenario file.
auto ShutDown(Open& open) -> void {
  FadeOutUninterrupted(open);
  StartSong(5);
  StopMusic(20);
  opeu_vga_clear();
  FadeToClosingPalette(open);
  RepeatUntil([&] { return FileExists(open, Near(open, Data(open).scenario_file_name)); });
  FreeBuffers(open);
}

auto OpenFiles(Open& open) -> void {
  Data(open).picture_handle = static_cast<int16_t>(OpenFileOrStop(open, Near(open, Data(open).picture_file_name)));
  Data(open).logo_handle    = static_cast<int16_t>(OpenFileOrStop(open, Near(open, Data(open).logo_file_name)));
}

auto CloseFiles(Open& open) -> void {
  opeu_dos_close(static_cast<int16_t>(Data(open).picture_handle));
  opeu_dos_close(static_cast<int16_t>(Data(open).logo_handle));
}

// The scenes from the prologue to the artillery, in order; whether one was aborted (the rest are then not played).
auto AnyScenesAborted(Open& open) -> bool {
  return PlayPrologueScene(open) != 0 || PlayColorCycleScene(open) != 0 || PlayFlickerScene(open) != 0 || PlayPanoramaScene(open) != 0 || PlayUnitsScene(open) != 0 || PlayBombingScene(open) != 0
         || PlayFlakScene(open) != 0 || PlayArtilleryScene(open) != 0;
}

auto RunOpening(Open& open) -> void {
  koei_install_dos_handlers();
  StartUp(open);
  OpenFiles(open);
  RepeatUntil([&] {
    opeu_fm_wait_start(20000);
    PlayKoeiScene(open);
    if (AnyScenesAborted(open)) {
      Data(open).aborted = 1;
      return true;
    }
    if (PlayTitle(open) != 0) {
      Data(open).aborted = 0;
      return true;
    }
    return false;
  });
  if (Data(open).aborted != 0) PlayTitle(open);
  CloseFiles(open);
  ShutDown(open);
  opeu_fm_noop();
}

}
