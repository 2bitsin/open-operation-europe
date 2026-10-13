// The arc of a shot between two screen points, drawn as small outlined boxes.
#include "map/btlline.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "core/limits.hpp"
#include "core/uiprim.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
constexpr uint16_t ARC_COLOR{ 0x17 };
// The arc's height above the straight line, in pixels.
constexpr int16_t ARC_RISE{ 15 };
constexpr int16_t ARC_FLATNESS{ -15 };

[[nodiscard]] auto Abs(int16_t v) -> int16_t {
  return static_cast<int16_t>(opeu_abs16_bits(static_cast<uint16_t>(v)));
}

void Outline(Game& game, int16_t x, int16_t y) {
  opeu_ui_outline(game, static_cast<const UiHost*>(port_uiprim_host()), x, y, x + 2, y + 4, ARC_COLOR);
}
} // namespace

// The arithmetic is on 16-bit words, as the original's: the square is truncated to 16 bits before the division.
void opeu_battle_draw_line(Game& game, ScreenPoint from, ScreenPoint to) {
  auto const x0{ static_cast<int16_t>(from.x) };
  auto const y0{ static_cast<int16_t>(from.y) };
  auto const x1{ static_cast<int16_t>(to.x) };
  auto const y1{ static_cast<int16_t>(to.y) };
  auto const dx{ static_cast<int16_t>(x1 - x0) };
  auto const dy{ static_cast<int16_t>(y1 - y0) };
  auto const width{ Abs(dx) };
  auto const height{ Abs(dy) };
  int16_t    slope{ static_cast<int16_t>((x1 > x0 && y1 > y0) || (x1 < x0 && y1 < y0) ? 1 : -1) };
  auto const half{ static_cast<int16_t>(width / 2) };
  auto const curve{ static_cast<int16_t>(static_cast<int16_t>(half * half) / ARC_FLATNESS) };
  int16_t    along{ static_cast<int16_t>(-half) };
  int16_t    step{ 1 };
  auto const lift{ [&] { return curve ? static_cast<int16_t>(static_cast<int16_t>(along * along) / curve) : int16_t{ 0 }; } };
  int16_t    x{ x0 };
  int16_t    y{ y0 };
  if (height < width) {
    if (x1 < x0) {
      slope = static_cast<int16_t>(-slope);
      step  = -1;
    }
    int16_t error{ static_cast<int16_t>(width / 2) };
    for (int16_t n{ width }; n; --n) {
      auto const px{ x };
      auto const py{ static_cast<int16_t>(y - lift() - ARC_RISE) };
      if (py >= 0) Outline(game, px, py);
      error = static_cast<int16_t>(error - height);
      if (error < 0) {
        error = static_cast<int16_t>(error + width);
        y     = static_cast<int16_t>(y + slope);
      }
      if (py >= 0) Outline(game, px, py);
      x = static_cast<int16_t>(x + step);
      ++along;
    }
  } else {
    if (y1 < y0) {
      slope = static_cast<int16_t>(-slope);
      step  = -1;
    }
    int16_t error{ static_cast<int16_t>(height / 2) };
    for (int16_t n{ height }; n; --n) {
      auto const px{ x };
      auto const py{ static_cast<int16_t>(y - lift() - ARC_RISE) };
      if (py >= 0) Outline(game, px, py);
      error = static_cast<int16_t>(error - width);
      if (error < 0) {
        error = static_cast<int16_t>(error + height);
        x     = static_cast<int16_t>(x + slope);
        ++along;
      }
      if (py >= 0) Outline(game, px, py);
      y = static_cast<int16_t>(y + step);
    }
  }
}
