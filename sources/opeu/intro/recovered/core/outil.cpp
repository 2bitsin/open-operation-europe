/* OPEN 062d:0008..041b (image 0x62d8..0x66ec): the opening's helper
 * segment. Its routines come from several MAIN files; the ones that are
 * byte-identical in OPEN keep MAIN's C text: the mouse services (MOUSE 05a8)
 * and the picture helpers (UIPRIM 0a04). The rest is OPEN's: a tick wait with
 * input abort, the image-stream variants that always read the picture file
 * DS:0440, the palette set/step/fade helpers on the current palette DS:0358,
 * the fade wrappers, the input poll and the open-or-die file opener.
 *
 * Near pointers are DS offsets; palettes are views of DGROUP's 48-byte rows.
 * The mouse driver, the FM driver's countdown, the files and the display are
 * called directly. The original spins forever after an open error; here the
 * spin pumps host events, so a window close or a headless run's end stops it.
 */
#include "recovered.hpp"

#include "hostinput.hpp" /* port_time_us */
#include "open-far.hpp"
#include "port.hpp"
#include "services.hpp"
#include "vgalib.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <span>

namespace opeu::intro::detail::outil {
namespace {
/* The mouse driver's state query (INT 33h AX=3). */
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

/* VGALIB's set_palette (OVGALIB 01AC) polls 3DAh bit 3 until the display is
 * out of vertical retrace and then until the next retrace starts, before
 * writing the DAC. The native opeu_set_palette leaves the wait out; MAIN does
 * not time anything by it, but OPEN's palette fades (one set_palette per step,
 * no other delay) are paced by it, one step per displayed frame. Platform edit
 * (timing): wait until the host clock passes the next frame boundary of mode
 * 12h's 59.940476 Hz refresh (the 25.175 MHz dot clock / (800 x 525)). The
 * boundaries are absolute multiples of the period, so the pump's 1 ms
 * granularity never accumulates drift. */
void WaitForRetrace() {
  constexpr uint64_t NUMERATOR{ static_cast<uint64_t>(800) * 525 * 1000000 };
  constexpr uint64_t DENOMINATOR{ 25175000 }; /* period in us = NUMERATOR / DENOMINATOR */
  uint64_t const     frame{ (port_time_us() * DENOMINATOR / NUMERATOR) + 1 };
  uint64_t const     next{ ((frame * NUMERATOR) + DENOMINATOR - 1) / DENOMINATOR };
  while (port_time_us() < next) port_pump();
}

/* The current palette DS:0358 into the DAC. */
void LoadPalette(Open& open) {
  WaitForRetrace();
  opeu_set_palette(0, 16, Data(open).palette);
}

/* Moves every current component one step toward the target; whether any
 * differed. */
auto StepToward(Open& open, Palette target) -> bool {
  std::span<uint8_t, 48> const current{ Data(open).palette };
  bool const                   dirty{ !std::ranges::equal(current, target) };
  std::ranges::transform(current, target, current.begin(), [](uint8_t now, uint8_t goal) { return static_cast<uint8_t>(now > goal ? now - 1 : now < goal ? now + 1 : now); });
  return dirty;
}

/* *stream++ through the far pointer DS:1c0e (offset), 1c10 (segment). */
auto StreamNext(Open& open) -> uint8_t {
  uint16_t const offset{ Data(open).stream_offset };
  Data(open).stream_offset = static_cast<uint16_t>(offset + 1);
  return FarByte(open, { Data(open).stream_segment, offset });
}

/* The picture decoder's reader is a plain function: the opening it reads
 * for, while one decode runs. */
Open* reading{ nullptr };

auto ReadStreamByte() -> uint8_t;

class StreamReading {
public:
  explicit StreamReading(Open& open) : _saved{ reading } {
    reading = &open;
  }
  StreamReading(StreamReading const&)                    = delete;
  StreamReading(StreamReading&&)                         = delete;
  auto operator=(StreamReading const&) -> StreamReading& = delete;
  auto operator=(StreamReading&&) -> StreamReading&      = delete;
  ~StreamReading() {
    reading = _saved;
  }

private:
  Open* _saved;
};
} // namespace

auto PollInput() -> int16_t;
auto NextPictureByte(Open& open) -> uint8_t;

namespace {
auto ReadStreamByte() -> uint8_t {
  return NextPictureByte(*reading);
}
} // namespace

/* 062d:0008. AX=ticks. Waits until the FM driver's down-counter has run
 * `ticks` further; 1 if the input poll reported a key or button first. */
auto WaitTicks(uint16_t ticks) -> int16_t {
  auto const target{ static_cast<uint16_t>(opeu_fm_busy() - ticks) };
  do {
    if (PollInput() != 0) return 1;
  } while (opeu_fm_busy() > target);
  return 0;
}

/* 062d:003e (MAIN 05a8:0210). The pointer's range: columns x..x+width, game
 * lines y..y+height (screen lines 40 lower). */
void LimitMouse(int16_t x, int16_t y, int16_t width, int16_t height) {
  port_mouse_x_range(static_cast<uint16_t>(x), static_cast<uint16_t>(width + x));
  port_mouse_y_range(static_cast<uint16_t>(y + 40), static_cast<uint16_t>(y + height + 40));
}

/* 062d:0094 (MAIN 05a8:000c). Driver reset, then limit to 640x400; returns the reset's result. */
auto ResetMouse([[maybe_unused]] Open& open) -> uint16_t {
  uint16_t const installed{ port_mouse_reset() };
  LimitMouse(0, 0, 640, 400);
  return installed;
}

/* 062d:00c6 (MAIN 05a8:0104). Queries the driver even without a mouse. */
auto LeftButtonDown() -> int {
  return ((QueryMouse().buttons & 1) != 0) ? -1 : 0;
}

/* 062d:00f0 (MAIN 05a8:012e). */
auto RightButtonDown() -> int {
  return ((QueryMouse().buttons & 2) != 0) ? -1 : 0;
}

/* 062d:011a (MAIN 05a8:0158). */
auto ButtonDown() -> int {
  return static_cast<int>((LeftButtonDown() != 0) || (RightButtonDown() != 0));
}

/* 062d:0132 (MAIN 0a04:0132 with a 0x5000-byte buffer at offset 0). */
auto NextPictureByte(Open& open) -> uint8_t {
  if (Data(open).stream_count == 0) {
    Data(open).stream_segment = Data(open).stream_buffer;
    Data(open).stream_offset  = 0;
    Data(open).stream_count   = static_cast<uint16_t>(ReadFar(open, Data(open).picture_handle, { Data(open).stream_buffer, 0, 0x5000 }));
  }
  Data(open).stream_count--;
  return StreamNext(open);
}

/* 062d:016a (MAIN 0a04:0250 on the picture file). DX:AX=position. */
void SeekPicture(Open& open, int32_t position) {
  if (opeu_dos_seek(Data(open).picture_handle, position, 0) != -1) Data(open).stream_count = 0;
}

/* 062d:018e (bytes of MAIN 0a04:0272). AX=x, DX=y, stack position; RETF 4. */
void DrawPicture(Open& open, uint16_t x, uint16_t y, int32_t position) {
  SeekPicture(open, position);
  StreamReading const decoding{ open };
  opeu_picture_decode_masked(x, y, ReadStreamByte);
}

/* 062d:01bc. Copies the palette to the current palette and loads the DAC. */
void SetCurrentPalette(Open& open, Palette palette) {
  std::ranges::copy(palette, std::begin(Data(open).palette));
  LoadPalette(open);
}

/* 062d:01fe. Moves every current component one step toward the target,
 * loads the DAC if anything changed and reports that. */
auto StepPalette(Open& open, Palette target) -> int16_t {
  if (!StepToward(open, target)) return 0;
  LoadPalette(open);
  return 1;
}

/* 062d:0262. AX=mode: fades completely. mode -1 waits for the driver timer
 * between steps; any other mode waits a tick and returns 1 if the input poll
 * aborts the fade. */
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

/* 062d:0304. Abortable fade to black. */
auto FadeOut(Open& open) -> int16_t {
  return FadePalette(open, Data(open).black, 2);
}

/* 062d:0310. Fade to black. */
auto FadeOutUninterrupted(Open& open) -> int16_t {
  return FadePalette(open, Data(open).black, 0xffff);
}

/* 062d:031c. Fade to DS:1944. */
auto FadeToClosingPalette(Open& open) -> int16_t {
  return FadePalette(open, Data(open).closing_palette, 0xffff);
}

/* 062d:0328. */
void SetIdentityPalette([[maybe_unused]] Open& open) {
  koei_vga_identity_palette();
}

/* 062d:032e. 1 (input flushed) for a key, 2 for a mouse button, else 0. */
auto PollInput() -> int16_t {
  if (opeu_turn_input_b() != 0) {
    FlushKeys();
    return 1;
  }
  if (ButtonDown() != 0) return 2;
  return 0;
}

/* 062d:0350. Opens the file named at a near pointer read/write or reports it
 * and stops. */
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
