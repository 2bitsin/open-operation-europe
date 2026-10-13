// Internal interface between the event pump (pump.cpp) and the keyboard and mouse driver state (input.cpp).
#pragma once

#include <cstdint>
#include <span>

// Monotonic time base of the services: host time, or virtual time when headless (advanced by the pump).
auto port_time_us() -> std::uint64_t;

namespace opeu::services::detail::hostinput {

inline constexpr auto FRAME_PIXELS{ 640uz * 480uz };
inline constexpr auto PALETTE_BYTES{ 256uz * 3uz };

// A button of the two-button driver changing state at a host position.
struct ButtonChange {
  int  button{}; // 0 left, 1 right
  bool down{};
  int  x{};
  int  y{};
};

// A key press as the BIOS keyboard interrupt stores it: AH=scan, AL=ASCII.
auto KeyPressed(std::uint8_t scan, std::uint8_t ascii) -> void;
// Host pointer position in screen pixels (0..639, 0..479).
auto MouseMoved(int x, int y) -> void;
auto MouseButton(ButtonChange const& change) -> void;

// Changes since the last drawing that need a new frame (pointer moved, shown or hidden).
[[nodiscard]] auto PointerDirty() -> bool;
// Overlays the driver's pointer on a rendered 640x480 frame of palette indices while it is shown; the palette receives
// the two pointer colours. True when a pointer was drawn.
auto DrawPointer(std::span<std::uint8_t, FRAME_PIXELS> frame, std::span<std::uint8_t, PALETTE_BYTES> palette) -> bool;

}

namespace opeu::services {
using detail::hostinput::ButtonChange;
using detail::hostinput::DrawPointer;
using detail::hostinput::KeyPressed;
using detail::hostinput::MouseButton;
using detail::hostinput::MouseMoved;
using detail::hostinput::PointerDirty;
}
