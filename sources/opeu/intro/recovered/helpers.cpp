// The opening's helpers: the uninterruptible delay, the tick wait with input abort, the mouse, the picture stream that always reads OPENGRP.NPK,
// the fades of the current palette, the input poll and the file opener that stops on a missing file. Near pointers are
// DGROUP offsets; palettes are views of DGROUP's 48-byte rows. The mouse driver, the FM driver's countdown, the files
// and the display are called directly.
#include "recovered.hpp"

#include "hostinput.hpp" // port_time_us
#include "open-services.hpp"
#include "port.hpp"
#include "segment-buffers.hpp"
#include "services.hpp"
#include "vgalib.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <span>

namespace opeu::intro::detail::helpers {
namespace {
// The mouse driver's state query.
struct MouseState {
  uint16_t buttons;
  uint16_t x;
  uint16_t y;
};

auto QueryMouse() -> MouseState {
  MouseState state{};
  port_mouse_state(&state.buttons, &state.x, &state.y);
  return state;
}

// The opening's palette fades load one palette per step and nothing else paces them: a step lasts one displayed frame,
// because a palette load waits for the next vertical retrace. The display service loads without waiting (the game times
// nothing by it), so the wait is here: until the host clock passes the next frame boundary of mode 12h's 59.940476 Hz
// refresh (the 25.175 MHz dot clock / (800 x 525)). The boundaries are absolute multiples of the period, so the pump's
// 1 ms granularity never accumulates drift.
void WaitForRetrace() {
  constexpr uint64_t NUMERATOR{ static_cast<uint64_t>(800) * 525 * 1000000 };
  constexpr uint64_t DENOMINATOR{ 25175000 }; // period in us = NUMERATOR / DENOMINATOR
  uint64_t const     frame{ (port_time_us() * DENOMINATOR / NUMERATOR) + 1 };
  uint64_t const     next{ ((frame * NUMERATOR) + DENOMINATOR - 1) / DENOMINATOR };
  while (port_time_us() < next) port_pump();
}

// The current palette into the DAC.
void LoadPalette(Open& open) {
  WaitForRetrace();
  opeu_set_palette(0, 16, Data(open).palette);
}

// Moves every current component one step toward the target; whether any differed.
auto StepToward(Open& open, Palette target) -> bool {
  std::span<uint8_t, 48> const current{ Data(open).palette };
  bool const                   dirty{ !std::ranges::equal(current, target) };
  std::ranges::transform(current, target, current.begin(), [](uint8_t now, uint8_t goal) { return static_cast<uint8_t>(now > goal ? now - 1 : now < goal ? now + 1 : now); });
  return dirty;
}

// The picture stream's next byte through its far pointer, which moves on.
auto StreamNext(Open& open) -> uint8_t {
  uint16_t const offset{ Data(open).stream_offset };
  Data(open).stream_offset = static_cast<uint16_t>(offset + 1);
  return FarByte(open, { Data(open).stream_segment, offset });
}

} // namespace

auto NextPictureByte(Open& open) -> uint8_t;

// Waits `ticks` of the effect countdown, whatever the input; 0 waits nothing.
auto Delay(uint16_t ticks) -> void {
  if (ticks != 0u) {
    services::StartEffectCountdown(ticks);
    while (services::EffectCountdown() != 0u);
  }
}

// Waits until the music timeline (the FM driver's down-counter) has run `ticks` further; 1 if a key or button came first.
auto WaitTicks(uint16_t ticks) -> int16_t {
  auto const target{ static_cast<uint16_t>(opeu_fm_busy() - ticks) };
  do {
    if (PollInput() != 0) return 1;
  } while (opeu_fm_busy() > target);
  return 0;
}

// The pointer's range: columns x..x+width, game lines y..y+height (screen lines 40 lower).
void LimitMouse(int16_t x, int16_t y, int16_t width, int16_t height) {
  port_mouse_x_range(static_cast<uint16_t>(x), static_cast<uint16_t>(width + x));
  port_mouse_y_range(static_cast<uint16_t>(y + 40), static_cast<uint16_t>(y + height + 40));
}

// Driver reset, then the pointer limited to 640x400; the reset's result.
auto ResetMouse([[maybe_unused]] Open& open) -> uint16_t {
  uint16_t const installed{ port_mouse_reset() };
  LimitMouse(0, 0, 640, 400);
  return installed;
}

// -1 while the left button is down, else 0; asks the driver even without a mouse.
auto LeftButtonDown() -> int {
  return ((QueryMouse().buttons & 1) != 0) ? -1 : 0;
}

// -1 while the right button is down, else 0.
auto RightButtonDown() -> int {
  return ((QueryMouse().buttons & 2) != 0) ? -1 : 0;
}

// Whether a mouse button is down.
auto ButtonDown() -> int {
  return static_cast<int>((LeftButtonDown() != 0) || (RightButtonDown() != 0));
}

// The picture stream's next byte; an empty stream first reads the next 0x5000 bytes of OPENGRP.NPK into its buffer.
auto NextPictureByte(Open& open) -> uint8_t {
  if (Data(open).stream_count == 0) {
    Data(open).stream_segment = Data(open).stream_buffer;
    Data(open).stream_offset  = 0;
    Data(open).stream_count   = static_cast<uint16_t>(ReadFar(open, Data(open).picture_handle, { Data(open).stream_buffer, 0, 0x5000 }));
  }
  Data(open).stream_count--;
  return StreamNext(open);
}

// Moves OPENGRP.NPK to a file position and empties the stream.
void SeekPicture(Open& open, int32_t position) {
  if (opeu_dos_seek(Data(open).picture_handle, position, 0) != -1) Data(open).stream_count = 0;
}

// Decodes the picture at a file position of OPENGRP.NPK onto the screen at (x column, y line).
void DrawPicture(Open& open, uint16_t x, uint16_t y, int32_t position) {
  SeekPicture(open, position);
  opeu_picture_decode_masked(x, y, [&open] { return NextPictureByte(open); });
}

// Copies the palette to the current palette and loads the DAC.
void SetCurrentPalette(Open& open, Palette palette) {
  std::ranges::copy(palette, std::begin(Data(open).palette));
  LoadPalette(open);
}

// Moves every current component one step toward the target, loads the DAC if anything changed and reports that.
auto StepPalette(Open& open, Palette target) -> int16_t {
  if (!StepToward(open, target)) return 0;
  LoadPalette(open);
  return 1;
}

// Fades completely. Mode FFFFh takes the steps back to back (Delay(0) waits nothing); any other mode waits a tick
// between steps and returns 1 when input aborts the fade.
auto FadePalette(Open& open, Palette target, uint16_t mode) -> int16_t {
  while (StepToward(open, target)) {
    LoadPalette(open);
    if (mode == 0xffff) {
      Delay(0);
    } else if (WaitTicks(1) != 0) {
      return 1;
    }
  }
  return 0;
}

// The fade to black that input aborts.
auto FadeOut(Open& open) -> int16_t {
  return FadePalette(open, Data(open).black, 2);
}

// The fade to black that nothing aborts.
auto FadeOutUninterrupted(Open& open) -> int16_t {
  return FadePalette(open, Data(open).black, 0xffff);
}

// The fade to the palette the opening leaves for the game.
auto FadeToClosingPalette(Open& open) -> int16_t {
  return FadePalette(open, Data(open).closing_palette, 0xffff);
}

// The attribute controller's identity palette.
void SetIdentityPalette([[maybe_unused]] Open& open) {
  koei_vga_identity_palette();
}

// 1 (the keys flushed) for a key, 2 for a mouse button, else 0.
auto PollInput() -> int16_t {
  if (opeu_turn_input_b() != 0) {
    FlushKeys();
    return 1;
  }
  if (ButtonDown() != 0) return 2;
  return 0;
}

// Opens the file named at a near pointer for reading and writing, or reports it and stops.
auto OpenFileOrStop(Open& open, uint16_t name) -> int16_t {
  auto const handle{ opeu_dos_open(reinterpret_cast<char const*>(&Byte(open, name)), 2) }; // NOLINT(*-reinterpret-cast): the DOS name's characters
  if (handle == -1) {
    opeu_vga_clear();
    CursorPrint(open, std::array<uint16_t, 2>{ Near(open, Data(open).error_format), name });
    Hang();
  }
  return handle;
}

void Hang() {
  for (;;) port_pump();
}

}
