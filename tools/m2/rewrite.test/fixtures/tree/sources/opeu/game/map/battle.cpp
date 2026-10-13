// The fixture's battle path: Calls keyed by BattleRoutine, a dispatcher that reorders one key's words, a forwarder,
// and the link handlers that list each routine's words.
#include "map/battle.hpp"
#include "common/battle-routine.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"

#include "port.hpp"

uint16_t opeu_battle_walk(uint8_t* ds, BattleEvent event, uint16_t element, uint16_t step) {
  Call(ds, event, BattleRoutine::SET_ORDER, element, step);
  return Call(ds, event, BattleRoutine::GRID_STEP, element, step);
}

uint16_t svc_battle(uint8_t* ds, uint32_t address, const uint16_t* args, unsigned count) {
  static constexpr unsigned char SET_ORDER_WORDS[]{ 1, 0 };
  switch (static_cast<BattleRoutine>(address)) {
  case BattleRoutine::SET_ORDER: return btl_reorder(ds, address, args, count, SET_ORDER_WORDS, 2);
  default:                       return port_service(ds, address, args, count);
  }
}

uint16_t svc_btlwalk(uint8_t* ds, uint32_t service, const uint16_t* args, unsigned count) {
  return svc_battle(ds, service, args, count);
}

auto port_routine_opeu_grid_step(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return opeu_grid_step(ds, Arg(args, count, 1), Arg(args, count, 0));
}

auto port_routine_opeu_set_order(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_set_order(ds, svc_btlwalk, Arg(args, count, 0), Arg(args, count, 1));
  return 0;
}

void opeu_battle_main(uint8_t* ds) {
  opeu_battle_walk(ds, svc_btlwalk, 3, 4);
  Call(ds, svc_btlwalk, BattleRoutine::UNLINKED, 1);
}
