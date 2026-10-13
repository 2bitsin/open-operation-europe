// What the game reads its player from: the BIOS keyboard, the two-button mouse driver (positions in screen pixels,
// the game band starting at y = 40) and the event pump that keeps the host responsive while the game polls.
#pragma once

#include "machine-types.hpp"

#include <cstdint>

namespace opeu::services::detail::machine_input {

struct MouseState {
  std::uint16_t buttons{}; // bit 0 left, bit 1 right
  std::uint16_t x{};
  std::uint16_t y{};

  auto operator==(MouseState const&) const -> bool = default;
};

class Input {
public:
  Input()                                = default;
  Input(Input const&)                    = delete;
  Input(Input&&)                         = delete;
  auto operator=(Input const&) -> Input& = delete;
  auto operator=(Input&&) -> Input&      = delete;
  virtual ~Input()                       = default;

  // ---- keyboard
  [[nodiscard]] virtual auto KeyWaiting() -> bool = 0;
  // The next key, scan code in the high byte and ASCII in the low; waits (pumping) for one.
  virtual auto ReadKey() -> std::uint16_t = 0;
  // The C runtime's getch(): the next key's ASCII; a key with ASCII 0 gives 0, then its scan code.
  virtual auto ReadCharacter() -> std::uint8_t = 0;
  // ---- mouse
  // Resets the driver; true when a mouse is installed.
  virtual auto               ResetMouse() -> bool                 = 0;
  virtual auto               ShowPointer() -> void                = 0;
  virtual auto               HidePointer() -> void                = 0;
  [[nodiscard]] virtual auto Mouse() -> MouseState                = 0;
  virtual auto               MovePointer(Point to) -> void        = 0;
  virtual auto               LimitPointerX(Range columns) -> void = 0;
  virtual auto               LimitPointerY(Range lines) -> void   = 0;
  // ---- the host
  // Handles the host's events, presents the frame when due and, headless, advances virtual time.
  virtual auto Pump() -> void = 0;
};

}

namespace opeu::services {
using detail::machine_input::Input;
using detail::machine_input::MouseState;
}
