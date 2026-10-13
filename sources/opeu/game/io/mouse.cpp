// The mouse services (INT 33h). input_mode is nonzero when a mouse is present. Mouse rows are offset by 40 pixels:
// positions report DX-40 and set or limit DX+40. Button predicates return -1 or 0 (BX bit 0 left, bit 1 right);
// opeu_turn_input_a is "any button".
#include "io/mouse.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/host.hpp"
#include "common/memory.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {

enum Function : std::uint16_t {
  RESET   = 0,
  SHOW    = 1,
  HIDE    = 2,
  STATUS  = 3, // position and buttons
  SET     = 4, // position CX, DX
  X_RANGE = 7, // CX..DX
  Y_RANGE = 8, // CX..DX
};

constexpr std::uint16_t ROW_OFFSET{ 40 };
constexpr std::uint16_t LEFT{ 1 };
constexpr std::uint16_t RIGHT{ 2 };
constexpr std::uint8_t  PRESSED{ 0x80 };
constexpr std::uint8_t  NO_MOUSE_CLICK{ 3 };

// INT 33h with AX = `function`; the registers come back.
auto Mouse(Game& game, HostService service, Function function, std::uint16_t cx = 0, std::uint16_t dx = 0) -> Regs {
  Regs r{};
  r.x.ax = function;
  r.x.cx = cx;
  r.x.dx = dx;
  Int86(game, service, r);
  return r;
}
} // namespace

// Resets the driver and limits the mouse to 640x400; returns the reset AX.
auto opeu_mouse_reset(Game& game, HostService service) -> std::uint16_t {
  auto const r{ Mouse(game, service, RESET) };
  opeu_mouse_window(game, service, 0, 0, 640, 400);
  return r.x.ax;
}

void opeu_mouse_show(Game& game, HostService service) {
  if (HasMouse(Data(game))) Mouse(game, service, SHOW);
}

void opeu_mouse_hide(Game& game, HostService service) {
  if (HasMouse(Data(game))) Mouse(game, service, HIDE);
}

auto opeu::game::detail::mouse::MousePosition(Game& game, HostService service) -> MousePoint {
  if (!HasMouse(Data(game))) return { 0, 0 };
  auto const r{ Mouse(game, service, STATUS) };
  return { r.x.cx, static_cast<std::uint16_t>(r.x.dx - ROW_OFFSET) };
}

// Stores the position at the DS words x and y, for the callers that pass their near addresses through a service.
void opeu_mouse_position(Game& game, HostService service, std::uint16_t x, std::uint16_t y) {
  auto const position{ MousePosition(game, service) };
  SetWord(game, x, position.x);
  SetWord(game, y, position.y);
}

void opeu_mouse_move(Game& game, HostService service, std::uint16_t x, std::uint16_t y) {
  if (HasMouse(Data(game))) Mouse(game, service, SET, x, static_cast<std::uint16_t>(y - ROW_OFFSET));
}

// Queries the driver even without a mouse.
auto opeu_mouse_left(Game& game, HostService service) -> int {
  return (Mouse(game, service, STATUS).x.bx & LEFT) ? -1 : 0;
}

auto opeu_mouse_right(Game& game, HostService service) -> int {
  return (Mouse(game, service, STATUS).x.bx & RIGHT) ? -1 : 0;
}

auto opeu_turn_input_a(Game& game, HostService service) -> int {
  return opeu_mouse_left(game, service) || opeu_mouse_right(game, service);
}

// 0 without a mouse or a button; else 0x80 | 1 left | 2 right.
auto opeu_mouse_buttons(Game& game, HostService service) -> std::uint8_t {
  if (!HasMouse(Data(game))) return 0;
  std::uint8_t buttons{ 0 };
  if (opeu_turn_input_a(game, service)) {
    buttons = PRESSED;
    if (opeu_mouse_left(game, service)) buttons = PRESSED | LEFT;
    if (opeu_mouse_right(game, service)) buttons |= RIGHT;
  }
  return buttons;
}

// Waits until every button is released.
void opeu_input_reset(Game& game, HostService service) {
  if (HasMouse(Data(game)))
    while (opeu_turn_input_a(game, service));
}

// Waits for a press and its release; 3 without a mouse.
auto opeu_mouse_wait_click(Game& game, HostService service) -> std::uint8_t {
  if (!HasMouse(Data(game))) return NO_MOUSE_CLICK;
  while (!opeu_turn_input_a(game, service));
  std::uint8_t buttons{ PRESSED };
  for (;;) {
    if (opeu_mouse_left(game, service)) {
      buttons |= LEFT;
      break;
    }
    if (opeu_mouse_right(game, service)) {
      buttons |= RIGHT;
      break;
    }
  }
  while (opeu_turn_input_a(game, service));
  return buttons;
}

void opeu_mouse_window(Game& game, HostService service, std::int16_t x, std::int16_t y, std::int16_t width, std::int16_t height) {
  Mouse(game, service, X_RANGE, static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(width + x));
  Mouse(game, service, Y_RANGE, static_cast<std::uint16_t>(y + ROW_OFFSET), static_cast<std::uint16_t>(y + height + ROW_OFFSET));
}

// The input driver call: int86(0x33) with the seven register words. The keyboard and dialog modules forward their
// driver calls here.
auto svc_mouse(Game&, std::uint16_t call, std::uint16_t* words) -> std::uint16_t {
  switch (call) {
  case INT86_CALL: return port_mouse_int33(words);
  default:         port_unhandled("mouse", call, 7);
  }
  return 0;
}
