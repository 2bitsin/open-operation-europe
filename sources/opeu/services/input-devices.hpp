#pragma once
// The BIOS keyboard (INT 16h) and the mouse driver (INT 33h) as objects: the type-ahead buffer the host's key presses
// fill, and the two-button driver the host's pointer moves. input.cpp defines them and, until services::Host owns
// them, keeps the process's one of each behind the game-facing functions (docs/platform-surface.md section 6).
//
// Keyboard: the BIOS type-ahead buffer holds 15 keys; each entry is the word AH=scan code, AL=ASCII (US layout, from
// the platform). A key pressed while the buffer is full is dropped (the BIOS beeps).
//
// Mouse: the functions MAIN issues, on a two-button driver in mode 12h (virtual screen 640x480):
//   AX=0 reset: AX=FFFFh (installed), BX=2 buttons; pointer hidden (counter -1), position centre (320,240), ranges the
//        full screen.
//   AX=1 show: counter+1 (at most 0); visible at 0.   AX=2 hide: counter-1.
//   AX=3 state: BX buttons (bit0 left, bit1 right), CX=x, DX=y.
//   AX=4 set position (clamped to the ranges).
//   AX=7/8 x/y range min..max (swapped when min>max); position clamped.
// The host pointer is absolute: each host motion moves the driver position to the host position clamped to the ranges.
// A press and release between two AX=3 queries is still reported once as pressed (the driver's state changes are
// asynchronous on DOS; a host frame can contain both events). The driver draws its pointer itself on the real
// hardware, never through the game's VRAM; here the pointer is drawn into the presented frame (Draw) while it is
// shown.

#include "hostinput.hpp"
#include "machine-input.hpp"
#include "machine-types.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace opeu::services::detail::input_devices {

// The BIOS type-ahead buffer: a ring of 16 words, 15 usable.
class KeyBuffer {
public:
  // Stores the key word AH=scan, AL=ASCII; a full buffer drops it.
  auto               Push(std::uint8_t scan, std::uint8_t ascii) -> void;
  [[nodiscard]] auto Waiting() const noexcept -> bool {
    return _head != _tail;
  }
  // The oldest key; the buffer must not be empty.
  [[nodiscard]] auto Take() -> std::uint16_t;

private:
  std::array<std::uint16_t, 16> _keys{};
  std::size_t                   _head{};
  std::size_t                   _tail{};
};

// A position of the host pointer in screen pixels.
struct HostPosition {
  int x{};
  int y{};
};

// A range of driver positions.
struct MouseRange {
  int min{};
  int max{};

  // The range a game call gives, its bounds as two's-complement words in either order.
  [[nodiscard]] static auto Between(Range given) noexcept -> MouseRange;
  [[nodiscard]] auto        Clamp(int value) const noexcept -> int;
};

// The two-button mouse driver.
class MouseDriver {
public:
  // The host pointer is absolute: the driver follows it within its ranges.
  auto Move(HostPosition to) noexcept -> void {
    SetPosition(to);
  }
  auto Button(ButtonChange const& change) noexcept -> void;
  auto Reset() noexcept -> void;
  auto Show() noexcept -> void;
  auto Hide() noexcept -> void;
  // AX=3: the buttons pressed now or since the last query, and the position.
  [[nodiscard]] auto State() noexcept -> MouseState;
  auto               SetPosition(HostPosition to) noexcept -> void;
  auto               SetXRange(MouseRange range) noexcept -> void;
  auto               SetYRange(MouseRange range) noexcept -> void;
  // Changes since the last Draw that need a new frame (pointer moved, shown or hidden).
  [[nodiscard]] auto Dirty() const noexcept -> bool;
  // Overlays the pointer on a 640x480 frame of palette entries while it is shown; the palette receives the pointer's
  // two colours. True when a pointer was drawn.
  auto Draw(std::span<std::uint8_t, hostinput::FRAME_PIXELS> frame, std::span<std::uint8_t, hostinput::PALETTE_BYTES> palette) -> bool;

private:
  auto DrawArrowRow(std::span<std::uint8_t, hostinput::FRAME_PIXELS> frame, int row) const -> void;

  int           _x{ 320 }; // driver position
  int           _y{ 240 };
  MouseRange    _x_range{ .min = 0, .max = 639 };
  MouseRange    _y_range{ .min = 0, .max = 479 };
  int           _counter{ -1 }; // 0 = visible
  std::uint32_t _buttons{};     // current host state
  std::uint32_t _latched{};     // pressed since the last AX=3
  int           _drawn_x{ -1 };
  int           _drawn_y{ -1 };
  bool          _drawn_visible{};
};

}

namespace opeu::services {
using detail::input_devices::HostPosition;
using detail::input_devices::KeyBuffer;
using detail::input_devices::MouseDriver;
using detail::input_devices::MouseRange;
}
