// The native input: each method forwards to its keyboard, mouse driver or event pump function.
#include "native-machine.hpp"

#include "services.hpp"

namespace opeu::services::detail::native_machine {
namespace {

constexpr std::uint16_t MOUSE_INSTALLED{ 0xFFFF };

}

auto NativeInput::KeyWaiting() -> bool {
  return opeu_turn_input_b() != 0;
}

auto NativeInput::ReadKey() -> std::uint16_t {
  return port_key_read();
}

auto NativeInput::ReadCharacter() -> std::uint8_t {
  return static_cast<std::uint8_t>(port_getch());
}

auto NativeInput::ResetMouse() -> bool {
  return port_mouse_reset() == MOUSE_INSTALLED;
}

auto NativeInput::ShowPointer() -> void {
  port_mouse_show();
}

auto NativeInput::HidePointer() -> void {
  port_mouse_hide();
}

auto NativeInput::Mouse() -> MouseState {
  MouseState state;
  port_mouse_state(&state.buttons, &state.x, &state.y);
  return state;
}

auto NativeInput::MovePointer(Point const to) -> void {
  port_mouse_set_position(to.x, to.y);
}

auto NativeInput::LimitPointerX(Range const columns) -> void {
  port_mouse_x_range(columns.min, columns.max);
}

auto NativeInput::LimitPointerY(Range const lines) -> void {
  port_mouse_y_range(lines.min, lines.max);
}

auto NativeInput::Pump() -> void {
  port_pump();
}

}
