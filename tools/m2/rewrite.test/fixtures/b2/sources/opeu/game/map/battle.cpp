// The fixture's battle module: routines that hand their event on, a dispatcher that only forwards, link handlers.
#include "map/battle.hpp"
#include "common/battle-routine.hpp"
#include "link/link.hpp"

void opeu_step(Game& game, BattleEvent event, std::uint16_t element) {
  opeu_mark(game, event, element);
}

void opeu_mark(Game& game, BattleEvent, std::uint16_t cell) {
  (void)game;
  (void)cell;
}

void opeu_show(Game& game, BattleEvent event) {
  opeu_step(game, event, 1);
}

std::uint16_t opeu_battle_turn(Game& game, BattleEvent event, std::uint16_t element) {
  opeu_step(game, event, element);
  opeu_show(game, event);
  return element;
}

std::uint16_t svc_battle(Game& game, std::uint32_t address, std::uint16_t const* args, unsigned count) {
  return port_service(game, address, args, count);
}

// The step through the link table.
auto port_routine_opeu_step(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_step(game, svc_battle, args[0]);
  return 0;
}

auto port_routine_opeu_mark(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_mark(game, svc_battle, args[0]);
  return 0;
}

auto port_routine_opeu_show(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_show(game, svc_battle);
  return 0;
}

void opeu_battle_main(Game& game) {
  opeu_battle_turn(game, svc_battle, 3);
}
