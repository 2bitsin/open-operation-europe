// The supply orders: a regiment supplies the regiment or the objective next to it. The transfer table is loaded
// from both, the transfer runs (the player's or the computer's) and the amounts go back. Each side of the table holds
// per resource the amount, the capacity and the computer's target.
#include "units/xferunit.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "core/limits.hpp"
#include "core/numeric.hpp"
#include "types.hpp"
#include "units/transfer-service.hpp"
#include "units/xferpnl.hpp"
#include "units/xferstep.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <array>
#include <cstddef>

using namespace opeu::game;

namespace {

using enum TransferService;

// The locals the services reach by near address, in each routine's reserve: the words the capacity and percentage
// services write, the two names and the target cell the player picks.
constexpr std::uint16_t SCRATCH_WORDS{ 6 };
constexpr std::uint16_t NAME_BYTES{ 0x20 };
constexpr std::uint16_t CELL_BYTES{ 2 };

// The computer's targets, in percent of the capacity: low, full, and what it adds to its side's fill.
constexpr std::uint16_t LOW_TARGET{ 10 };
constexpr std::uint16_t FULL_TARGET{ 100 };
constexpr std::uint16_t FILL_MARGIN{ 30 };

// The scratch word n of a reserve.
auto Scratch(ServiceLocals const& words, unsigned n) -> std::uint16_t {
  return words.At(static_cast<std::uint16_t>(2 * n));
}

// The near pointer of a transfer_table word.
constexpr auto TableAt(unsigned side, unsigned row, unsigned column) -> std::uint16_t {
  return static_cast<std::uint16_t>(offsetof(DataSegment, transfer_table) + 2 * ((side * 3u + row) * 3u + column));
}

// Whether the computer has taken over the regiment's division (it runs that division's transfers).
auto DivisionHeld(Game& game, HostWordsEvent event, std::uint16_t unit) -> bool {
  return Record<Group>(Data(game), Call(game, event, UNIT_GROUP, unit)).flags & Bits(GroupFlag::AI_MARK);
}

auto UnitName(Game& game, HostWordsEvent event, std::uint16_t unit, std::uint16_t buffer) -> std::uint16_t {
  return Call(game, event, UNIT_NAME, unit, buffer);
}

// Writes the table's amounts back to the regiment.
void StoreUnit(DataSegment& data, std::uint16_t unit, std::uint8_t side) {
  auto& regiment{ Record<Unit>(data, unit) };
  for (std::uint8_t resource = 0; resource < RESOURCES; ++resource) regiment.resources[resource] = data.transfer_table[side][resource][AMOUNT];
}

// The supply order next to the current regiment, of an objective or of another regiment.
auto SupplyOrder(Game& game, HostWordsEvent event, bool objective) -> std::uint16_t {
  ServiceLocals const cell{ CELL_BYTES };
  auto const          x_at{ cell.At(0) };
  auto const          y_at{ cell.At(1) };
  auto&               data{ Data(game) };
  auto const          target{ objective ? Call(game, event, OBJECTIVE_TARGET, x_at, y_at, data.current_side) : Call(game, event, UNIT_TARGET, x_at, y_at) };
  if (target) {
    if (!Call(game, event, ADJACENT, Byte(game, x_at), Byte(game, y_at))) return 0;
    auto const hours{ opeu_transfer_level(game, event) };
    // The order takes its hours before the supplies move.
    if (!Call(game, event, ORDER_PROGRESS, Near(data, Record<Unit>(data, data.current_unit_ptr).orders[0]), hours)) return 1;
    auto const unit{ data.current_unit_ptr };
    if (objective)
      opeu_transfer_objective(game, event, unit, target);
    else
      opeu_transfer_units(game, event, unit, target);
  }
  Call(game, event, RESTORE_ORDER, data.current_unit_ptr);
  return 1;
}
} // namespace

// Loads a regiment into the table's `side`: its amounts, its capacities and the computer's targets in percent of
// each capacity. The division's eighth regiment fills to 10%, or beside an objective to 100% of a resource the
// objective holds above its reserve; other regiments fill to their side's percentage plus 30, at most 100.
void opeu_transfer_load_unit(Game& game, HostWordsEvent event, uint16_t unit, uint8_t side) {
  ServiceLocals const scratch{ 2 * SCRATCH_WORDS };
  auto&               data{ Data(game) };
  auto const&         regiment{ Record<Unit>(data, unit) };
  opeu_transfer_set(game, side, 0, AMOUNT, regiment.resources[0]);
  opeu_transfer_set(game, side, 1, AMOUNT, regiment.resources[1]);
  opeu_transfer_set(game, side, 2, AMOUNT, regiment.resources[2]);
  std::array<std::uint16_t, RESOURCES> const words{ Scratch(scratch, 1), Scratch(scratch, 5), Scratch(scratch, 3) };
  Call(game, event, UNIT_CAPACITIES, unit, words[0], words[1], words[2]);
  std::array<std::uint16_t, RESOURCES> const capacity{ Word(game, words[0]), Word(game, words[1]), Word(game, words[2]) };
  for (std::uint8_t resource = 0; resource < RESOURCES; ++resource) opeu_transfer_set(game, side, resource, CAPACITY, capacity[resource]);
  std::array<std::uint16_t, RESOURCES> target;
  if (Call(game, event, LAST_SLOT, static_cast<std::uint8_t>(Call(game, event, UNIT_SLOT, unit)))) {
    if (!data.transfer_objective_ptr) {
      target.fill(LOW_TARGET);
    } else {
      auto const& objective{ Record<Objective>(data, data.transfer_objective_ptr) };
      for (std::uint8_t resource = 0; resource < RESOURCES; ++resource) target[resource] = objective.resources[resource] > objective.reserves[resource] ? FULL_TARGET : LOW_TARGET;
    }
  } else {
    std::array<std::uint16_t, RESOURCES> const fill{ Scratch(scratch, 0), Scratch(scratch, 4), Scratch(scratch, 2) };
    Call(game, event, SIDE_PERCENTAGES, data.current_side, fill[0], fill[1], fill[2], 0);
    for (std::uint8_t resource = 0; resource < RESOURCES; ++resource) target[resource] = opeu_min16(static_cast<std::uint16_t>(Word(game, fill[resource]) + FILL_MARGIN), FULL_TARGET);
  }
  for (std::uint8_t resource = 0; resource < RESOURCES; ++resource) opeu_transfer_set(game, side, resource, TARGET, opeu_percent(capacity[resource], target[resource]));
}

// Loads an objective into the table's right side: its capacities, its reserves as the targets and its amounts.
void opeu_transfer_load_objective(Game& game, HostWordsEvent event, uint16_t objective) {
  auto& data{ Data(game) };
  Call(game, event, OBJECTIVE_CAPACITIES, objective, TableAt(1, 0, CAPACITY), TableAt(1, 1, CAPACITY), TableAt(1, 2, CAPACITY));
  auto const& city{ Record<Objective>(data, objective) };
  for (std::uint8_t resource = 0; resource < RESOURCES; ++resource) data.transfer_table[1][resource][TARGET] = city.reserves[resource];
  for (std::uint8_t resource = 0; resource < RESOURCES; ++resource) data.transfer_table[1][resource][AMOUNT] = city.resources[resource];
}

// The transfer between two regiments, repeated until it is confirmed (or the computer finishes); the player runs it
// when a player commands the side, unless the computer has taken over both divisions.
void opeu_transfer_units(Game& game, HostWordsEvent event, uint16_t first, uint16_t second) {
  ServiceLocals const names{ 2 * NAME_BYTES };
  auto&               data{ Data(game) };
  data.transfer_objective_ptr = 0;
  do {
    opeu_transfer_load_unit(game, event, first, 0);
    opeu_transfer_load_unit(game, event, second, 1);
    // The second name is taken first (the original's right-to-left arguments).
    auto const second_name{ UnitName(game, event, second, names.At(NAME_BYTES)) };
    auto const first_name{ UnitName(game, event, first, names.At(0)) };
    opeu_transfer_draw(game, event, first_name, second_name);
  } while (!opeu_transfer_run(game, event, data.side_control[data.current_side] == SIDE_PLAYED && (!DivisionHeld(game, event, first) || !DivisionHeld(game, event, second))));
  StoreUnit(data, first, 0);
  StoreUnit(data, second, 1);
}

// The transfer between a regiment and an objective.
void opeu_transfer_objective(Game& game, HostWordsEvent event, uint16_t unit, uint16_t objective) {
  ServiceLocals const name{ NAME_BYTES };
  auto&               data{ Data(game) };
  data.transfer_objective_ptr = objective;
  do {
    opeu_transfer_load_unit(game, event, unit, 0);
    opeu_transfer_load_objective(game, event, objective);
    opeu_transfer_draw(game, event, UnitName(game, event, unit, name.At(0)), objective);
  } while (!opeu_transfer_run(game, event, data.side_control[data.current_side] == SIDE_PLAYED && !DivisionHeld(game, event, unit)));
  StoreUnit(data, unit, 0);
  auto& city{ Record<Objective>(data, objective) };
  for (std::uint8_t resource = 0; resource < RESOURCES; ++resource) city.resources[resource] = data.transfer_table[1][resource][AMOUNT];
  Call(game, event, DAILY_CONTEXT, data.current_side);
}

// The hours a supply order of the current regiment takes: 4, plus its experience / 24, less its training / 32.
uint8_t opeu_transfer_level(Game& game, HostWordsEvent event) {
  auto const unit{ Data(game).current_unit_ptr };
  auto const training{ static_cast<std::uint16_t>(Call(game, event, UNIT_TRAINING, unit) >> 5) };
  return static_cast<std::uint8_t>(Call(game, event, UNIT_EXPERIENCE, unit) / 24 - training + 4);
}

uint16_t opeu_order_supply_objective(Game& game, HostWordsEvent event) {
  return SupplyOrder(game, event, true);
}

uint16_t opeu_order_supply_unit(Game& game, HostWordsEvent event) {
  return SupplyOrder(game, event, false);
}

uint16_t svc_xferunit(Game& game, uint16_t id, uint16_t* args, uint16_t count) {
  return svc_transfer(game, "xferunit", id, args, count);
}

auto port_routine_opeu_order_supply_objective(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return opeu_order_supply_objective(game, svc_xferunit);
}

auto port_routine_opeu_order_supply_unit(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return opeu_order_supply_unit(game, svc_xferunit);
}
