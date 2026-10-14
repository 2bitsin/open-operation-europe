// BIOS keyboard (INT 16h) and mouse driver (INT 33h) natively: the devices of input-devices.hpp, the process's one
// of each, and the game-facing functions over them. AH=01h of INT 16h reports whether a key is waiting, AH=00h removes
// and returns one, waiting (and pumping host events) while the buffer is empty.
#include "hostinput.hpp"
#include "input-devices.hpp"
#include "port.hpp"
#include "services.hpp"

#include <algorithm>
#include <array>
#include <ranges>
#include <string_view>

namespace opeu::services::detail::input_devices {
namespace {

// The default arrow of the Microsoft driver: '#' outline (black), '.' body (white), hot spot at the tip.
constexpr std::array<std::string_view, 16> ARROW{
  "#", "##", "#.#", "#..#", "#...#", "#....#", "#.....#", "#......#", "#.......#", "#........#", "#.....#####", "#..#..#", "#.# #..#", "##  #..#", "     #..#", "     ####",
};

constexpr std::uint8_t POINTER_BLACK{ 254 };
constexpr std::uint8_t POINTER_WHITE{ 255 };
constexpr auto         SCREEN_WIDTH{ 640 };
constexpr auto         SCREEN_HEIGHT{ 480 };
constexpr auto         FRAME_WIDTH{ 640uz }; // a frame line's pixels

}

auto KeyBuffer::Push(std::uint8_t const scan, std::uint8_t const ascii) -> void {
  auto const next{ (_tail + 1) % _keys.size() };
  if (next == _head) {
    port_trace("svc", "key %02x:%02x dropped (buffer full)", scan, ascii);
    return;
  }
  _keys.at(_tail) = static_cast<std::uint16_t>((scan << 8) | ascii);
  _tail           = next;
}

auto KeyBuffer::Take() -> std::uint16_t {
  auto const key{ _keys.at(_head) };
  _head = (_head + 1) % _keys.size();
  return key;
}

auto MouseRange::Between(Range const given) noexcept -> MouseRange {
  auto const first{ int{ static_cast<std::int16_t>(given.min) } };
  auto const second{ int{ static_cast<std::int16_t>(given.max) } };
  return { .min = std::min(first, second), .max = std::max(first, second) };
}

auto MouseRange::Clamp(int const value) const noexcept -> int {
  return std::clamp(value, min, max);
}

auto MouseDriver::Button(ButtonChange const& change) noexcept -> void {
  if (change.button < 0 || change.button > 1) return; // two-button driver
  Move({ .x = change.x, .y = change.y });
  auto const bit{ 1u << change.button };
  if (change.down) {
    _buttons |= bit;
    _latched |= bit;
  } else {
    _buttons &= ~bit;
  }
}

auto MouseDriver::Reset() noexcept -> void {
  _counter = -1;
  _x_range = { .min = 0, .max = SCREEN_WIDTH - 1 };
  _y_range = { .min = 0, .max = SCREEN_HEIGHT - 1 };
  _x       = SCREEN_WIDTH / 2;
  _y       = SCREEN_HEIGHT / 2;
  _latched = 0;
}

auto MouseDriver::Show() noexcept -> void {
  if (_counter < 0) ++_counter;
}

auto MouseDriver::Hide() noexcept -> void {
  --_counter;
}

auto MouseDriver::State() noexcept -> MouseState {
  auto const state{
    MouseState{ .buttons = static_cast<std::uint16_t>((_buttons | _latched) & 3), .x = static_cast<std::uint16_t>(_x), .y = static_cast<std::uint16_t>(_y) }
  };
  _latched = 0;
  return state;
}

auto MouseDriver::SetPosition(HostPosition const to) noexcept -> void {
  _x = _x_range.Clamp(to.x);
  _y = _y_range.Clamp(to.y);
}

auto MouseDriver::SetXRange(MouseRange const range) noexcept -> void {
  _x_range = range;
  SetPosition({ .x = _x, .y = _y });
}

auto MouseDriver::SetYRange(MouseRange const range) noexcept -> void {
  _y_range = range;
  SetPosition({ .x = _x, .y = _y });
}

auto MouseDriver::Dirty() const noexcept -> bool {
  auto const visible{ _counter >= 0 };
  return visible != _drawn_visible || (visible && (_x != _drawn_x || _y != _drawn_y));
}

auto MouseDriver::Draw(std::span<std::uint8_t, hostinput::FRAME_PIXELS> const frame, std::span<std::uint8_t, hostinput::PALETTE_BYTES> const palette) -> bool {
  _drawn_visible = _counter >= 0;
  _drawn_x       = _x;
  _drawn_y       = _y;
  if (!_drawn_visible) return false;
  std::ranges::fill(palette.subspan(POINTER_BLACK * 3uz, 3), 0);
  std::ranges::fill(palette.subspan(POINTER_WHITE * 3uz, 3), 255);
  std::ranges::for_each(std::views::iota(0, static_cast<int>(ARROW.size())), [&](int const row) { DrawArrowRow(frame, row); });
  return true;
}

auto MouseDriver::DrawArrowRow(std::span<std::uint8_t, hostinput::FRAME_PIXELS> const frame, int const row) const -> void {
  auto const y{ _y + row };
  if (y < 0 || y >= SCREEN_HEIGHT) return;
  auto const line{ ARROW.at(static_cast<std::size_t>(row)) };
  std::ranges::for_each(std::views::iota(0, static_cast<int>(line.size())), [&](int const column) {
    auto const x{ _x + column };
    auto const mark{ line.at(static_cast<std::size_t>(column)) };
    if (x < 0 || x >= SCREEN_WIDTH || mark == ' ') return;
    frame[(static_cast<std::size_t>(y) * FRAME_WIDTH) + static_cast<std::size_t>(x)] = mark == '#' ? POINTER_BLACK : POINTER_WHITE;
  });
}

}

namespace opeu::services::detail::hostinput {
namespace {

// The devices of the process (legacy: the INT 16h and INT 33h entry points below are free functions).
auto TheKeyboard() -> KeyBuffer& {
  static KeyBuffer keyboard;
  return keyboard;
}

auto TheMouse() -> MouseDriver& {
  static MouseDriver mouse;
  return mouse;
}

}

auto KeyPressed(std::uint8_t const scan, std::uint8_t const ascii) -> void {
  TheKeyboard().Push(scan, ascii);
}

auto MouseMoved(int const x, int const y) -> void {
  TheMouse().Move({ .x = x, .y = y });
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
  TheMouse().SetPosition({ .x = static_cast<std::int16_t>(x), .y = static_cast<std::int16_t>(y) });
}

void port_mouse_x_range(uint16_t min, uint16_t max) {
  TheMouse().SetXRange(opeu::services::MouseRange::Between({ .min = min, .max = max }));
}

void port_mouse_y_range(uint16_t min, uint16_t max) {
  TheMouse().SetYRange(opeu::services::MouseRange::Between({ .min = min, .max = max }));
}
