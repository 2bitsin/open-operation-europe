// The fixture's menu.
#include "core/menu.hpp"
#include "core/draw.hpp"

namespace {
// The services of the menu.
enum Service : std::uint16_t {
  SHOW = 0, // (row)
  DRAW = 1, // (row)
};

// The ids the hold loop still calls by number.
enum class HoldService : std::uint16_t {
  WAIT = 0,
};

// The id the menu asked a row for.
constexpr std::uint16_t ROW_SERVICE{ 4 }; // (row)
// The id the hold still answers.
constexpr std::uint16_t HOLD_SERVICE{ 5 };

using Hook = void (*)(Game& game, HostEvent event);
} // namespace

void opeu_menu_show(Game& game, HostEvent event, std::uint16_t row) {
  opeu_draw_row(game, row);
}

void opeu_menu_silent(Game& game, HostEvent event) {
  (void)event;
}

void opeu_menu_run(Game& game, HostEvent event) {
  opeu_menu_show(game, event, 1);
  opeu_menu_silent(game, event);
}

void opeu_menu_hold(Game& game, HostEvent event) {
  Call(game, event, HoldService::WAIT);
}

void opeu_menu_hooked(Game& game, HostEvent event) {
  opeu_draw_row(game, 0);
}

Hook const hook{ opeu_menu_hooked };

std::uint16_t svc_menu(Game& game, std::uint16_t id, std::uint16_t const* args, std::uint16_t count) {
  switch (id) {
  case SHOW: opeu_menu_show(game, svc_menu, args[0]); break;
  case DRAW: opeu_draw_row(game, args[0]); break;
  }
  return 0;
}

// An event written as a function: only the argument the pass drops names it.
std::uint16_t MenuEvent(Game& game, std::uint16_t id, std::uint16_t const* args, std::uint16_t count) {
  return 0;
}

std::uint16_t svc_hold(Game& game, std::uint16_t id, std::uint16_t const* args, std::uint16_t count) {
  return id == HOLD_SERVICE ? 1 : 0;
}

void opeu_menu_entry(Game& game) {
  opeu_menu_run(game, svc_menu);
  opeu_menu_run(game, MenuEvent);
  opeu_menu_hold(game, svc_hold);
}
