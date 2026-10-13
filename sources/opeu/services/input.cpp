// BIOS keyboard (INT 16h) and mouse driver (INT 33h) natively.
//
// Keyboard: the BIOS type-ahead buffer holds 15 keys; each entry is the word AH=scan code, AL=ASCII (US layout, from
// the platform). AH=01h reports whether a key is waiting, AH=00h removes and returns one, waiting (and pumping host
// events) while the buffer is empty. A key pressed while the buffer is full is dropped (the BIOS beeps).
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
// hardware, never through the game's VRAM; here the pointer is drawn into the presented frame (DrawPointer) while it is
// shown.
#include "hostinput.hpp"
#include "port.hpp"
#include "services.hpp"

#include <algorithm>
#include <array>
#include <optional>
#include <ranges>
#include <string_view>

namespace opeu::services::detail::hostinput {
namespace {

// The BIOS type-ahead buffer: a ring of 16 words, 15 usable.
class Keyboard {
public:
  auto Push(std::uint8_t const scan, std::uint8_t const ascii) -> void {
    auto const next{ (_tail + 1) % _keys.size() };
    if (next == _head) {
      port_trace("svc", "key %02x:%02x dropped (buffer full)", scan, ascii);
      return;
    }
    _keys[_tail] = static_cast<std::uint16_t>(scan << 8 | ascii);
    _tail        = next;
  }

  [[nodiscard]] auto Waiting() const -> bool {
    return _head != _tail;
  }

  // The oldest key; the buffer must not be empty.
  [[nodiscard]] auto Take() -> std::uint16_t {
    auto const key{ _keys[_head] };
    _head = (_head + 1) % _keys.size();
    return key;
  }

private:
  std::array<std::uint16_t, 16> _keys{};
  std::size_t                   _head{};
  std::size_t                   _tail{};
};

// A range of driver positions, the bounds in either order.
struct Range {
  int min;
  int max;

  [[nodiscard]] static auto Between(std::uint16_t const a, std::uint16_t const b) -> Range {
    auto const first{ int{ static_cast<std::int16_t>(a) } };
    auto const second{ int{ static_cast<std::int16_t>(b) } };
    return { .min = std::min(first, second), .max = std::max(first, second) };
  }
  [[nodiscard]] auto Clamp(int const value) const -> int {
    return std::clamp(value, min, max);
  }
};

// The driver's position as AX=3 reports it.
struct MouseState {
  std::uint16_t buttons;
  std::uint16_t x;
  std::uint16_t y;
};

// The default arrow of the Microsoft driver: '#' outline (black), '.' body (white), hot spot at the tip.
constexpr std::array<std::string_view, 16> ARROW{
  "#", "##", "#.#", "#..#", "#...#", "#....#", "#.....#", "#......#", "#.......#", "#........#", "#.....#####", "#..#..#", "#.# #..#", "##  #..#", "     #..#", "     ####",
};

constexpr std::uint8_t POINTER_BLACK{ 254 };
constexpr std::uint8_t POINTER_WHITE{ 255 };

// The two-button mouse driver.
class Mouse {
public:
  // The host pointer is absolute: the driver follows it within its ranges.
  auto Move(int const x, int const y) -> void {
    SetPosition(x, y);
  }

  auto Button(ButtonChange const& change) -> void {
    if (change.button < 0 || change.button > 1) return; // two-button driver
    Move(change.x, change.y);
    auto const bit{ 1u << change.button };
    if (change.down) {
      _buttons |= bit;
      _latched |= bit;
    } else {
      _buttons &= ~bit;
    }
  }

  auto Reset() -> void {
    _counter = -1;
    _x_range = { .min = 0, .max = 639 };
    _y_range = { .min = 0, .max = 479 };
    _x       = 320;
    _y       = 240;
    _latched = 0;
  }

  auto Show() -> void {
    if (_counter < 0) ++_counter;
  }

  auto Hide() -> void {
    --_counter;
  }

  // AX=3: the buttons pressed now or since the last query, and the position.
  [[nodiscard]] auto State() -> MouseState {
    auto const state{
      MouseState{ .buttons = static_cast<std::uint16_t>((_buttons | _latched) & 3), .x = static_cast<std::uint16_t>(_x), .y = static_cast<std::uint16_t>(_y) }
    };
    _latched = 0;
    return state;
  }

  auto SetPosition(int const x, int const y) -> void {
    _x = _x_range.Clamp(x);
    _y = _y_range.Clamp(y);
  }

  auto SetXRange(Range const range) -> void {
    _x_range = range;
    SetPosition(_x, _y);
  }

  auto SetYRange(Range const range) -> void {
    _y_range = range;
    SetPosition(_x, _y);
  }

  [[nodiscard]] auto Dirty() const -> bool {
    auto const visible{ _counter >= 0 };
    return visible != _drawn_visible || (visible && (_x != _drawn_x || _y != _drawn_y));
  }

  auto Draw(std::span<std::uint8_t, FRAME_PIXELS> const frame, std::span<std::uint8_t, PALETTE_BYTES> const palette) -> bool {
    _drawn_visible = _counter >= 0;
    _drawn_x       = _x;
    _drawn_y       = _y;
    if (!_drawn_visible) return false;
    std::ranges::fill(palette.subspan(POINTER_BLACK * 3uz, 3), 0);
    std::ranges::fill(palette.subspan(POINTER_WHITE * 3uz, 3), 255);
    std::ranges::for_each(std::views::iota(0, static_cast<int>(ARROW.size())), [&](int const row) { DrawArrowRow(frame, row); });
    return true;
  }

private:
  auto DrawArrowRow(std::span<std::uint8_t, FRAME_PIXELS> const frame, int const row) const -> void {
    auto const y{ _y + row };
    if (y < 0 || y >= 480) return;
    auto const line{ ARROW[static_cast<std::size_t>(row)] };
    std::ranges::for_each(std::views::iota(0, static_cast<int>(line.size())), [&](int const column) {
      auto const x{ _x + column };
      auto const mark{ line[static_cast<std::size_t>(column)] };
      if (x < 0 || x >= 640 || mark == ' ') return;
      frame[static_cast<std::size_t>(y * 640 + x)] = mark == '#' ? POINTER_BLACK : POINTER_WHITE;
    });
  }

  int           _x{ 320 }; // driver position
  int           _y{ 240 };
  Range         _x_range{ .min = 0, .max = 639 };
  Range         _y_range{ .min = 0, .max = 479 };
  int           _counter{ -1 }; // 0 = visible
  std::uint32_t _buttons{};     // current host state
  std::uint32_t _latched{};     // pressed since the last AX=3
  int           _drawn_x{ -1 };
  int           _drawn_y{ -1 };
  bool          _drawn_visible{};
};

// The devices of the process (legacy: the INT 16h and INT 33h entry points below are free functions).
auto TheKeyboard() -> Keyboard& {
  static Keyboard keyboard;
  return keyboard;
}

auto TheMouse() -> Mouse& {
  static Mouse mouse;
  return mouse;
}

}

auto KeyPressed(std::uint8_t const scan, std::uint8_t const ascii) -> void {
  TheKeyboard().Push(scan, ascii);
}

auto MouseMoved(int const x, int const y) -> void {
  TheMouse().Move(x, y);
}

auto MouseButton(ButtonChange const& change) -> void {
  TheMouse().Button(change);
}

auto PointerDirty() -> bool {
  return TheMouse().Dirty();
}

auto DrawPointer(std::span<std::uint8_t, FRAME_PIXELS> const frame, std::span<std::uint8_t, PALETTE_BYTES> const palette) -> bool {
  return TheMouse().Draw(frame, palette);
}

}

using opeu::services::detail::hostinput::TheKeyboard;
using opeu::services::detail::hostinput::TheMouse;

auto port_key_available() -> uint16_t {
  port_pump();
  return TheKeyboard().Waiting() ? 1 : 0;
}

auto port_key_read() -> uint16_t {
  // A hand loop: the key arrives from the host while the pump runs; how many pumps that takes is not known.
  while (!TheKeyboard().Waiting()) port_pump();
  auto const key{ TheKeyboard().Take() };
  port_trace("svc", "int16 read -> %04x", key);
  return key;
}

auto port_mouse_reset() -> uint16_t {
  TheMouse().Reset();
  port_trace("svc", "int33 reset");
  return 0xFFFF;
}

void port_mouse_show() {
  TheMouse().Show();
}

void port_mouse_hide() {
  TheMouse().Hide();
}

void port_mouse_state(uint16_t* buttons, uint16_t* x, uint16_t* y) {
  port_pump();
  auto const state{ TheMouse().State() };
  *buttons = state.buttons;
  *x       = state.x;
  *y       = state.y;
}

void port_mouse_set_position(uint16_t x, uint16_t y) {
  TheMouse().SetPosition(static_cast<std::int16_t>(x), static_cast<std::int16_t>(y));
}

void port_mouse_x_range(uint16_t min, uint16_t max) {
  TheMouse().SetXRange(opeu::services::detail::hostinput::Range::Between(min, max));
}

void port_mouse_y_range(uint16_t min, uint16_t max) {
  TheMouse().SetYRange(opeu::services::detail::hostinput::Range::Between(min, max));
}
