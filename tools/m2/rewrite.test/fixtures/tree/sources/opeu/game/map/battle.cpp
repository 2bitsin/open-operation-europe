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

// A battle dispatcher that serves two routine addresses itself (one in a single statement, one in several) and hands
// the others to the link table.
uint16_t svc_battle_guard(uint8_t* ds, uint32_t address, const uint16_t* args, unsigned count) {
  if (address == std::to_underlying(BattleRoutine::PATH_STEP)) {
    return opeu_path_step(ds, Arg(args, count, 0), Arg(args, count, 1));
  }
  if (address == std::to_underlying(BattleRoutine::PATH_MARK)) {
    opeu_path_mark(ds, Arg(args, count, 0));
    return opeu_path_mark(ds, Arg(args, count, 1));
  }
  return port_service(ds, address, args, count);
}

// Handlers of two statements: a local of the routine's result returned, a local of a word passed on.
auto port_routine_opeu_ask(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  auto const answer{ static_cast<std::uint16_t>(opeu_ask(ds, Arg(args, count, 0))) };
  return answer;
}

auto port_routine_opeu_mark_cell(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  auto const cell{ Arg(args, count, 0) };
  opeu_mark_cell(ds, cell);
  return 0;
}

void opeu_battle_guarded(uint8_t* ds) {
  auto const step{ Call(ds, svc_battle_guard, BattleRoutine::PATH_STEP, 5, 6) };
  Call(ds, svc_battle_guard, BattleRoutine::PATH_MARK, step);
  auto const yes{ Call(ds, svc_battle_guard, BattleRoutine::ASK, 8) };
  Call(ds, svc_battle_guard, BattleRoutine::MARK_CELL, yes);
}
