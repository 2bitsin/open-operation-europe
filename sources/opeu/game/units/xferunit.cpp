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

#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <array>
#include <cstddef>

using namespace opeu::game;

namespace {

using enum TransferService;

// Fixed spots in the DS stack window: the words the capacity and percentage services write, the two names and the
// target cell the player picks.
constexpr std::uint16_t SCRATCH_AT{ 0xee00 };
constexpr std::uint16_t FIRST_NAME_AT{ 0xee20 };
constexpr std::uint16_t SECOND_NAME_AT{ 0xee40 };
constexpr std::uint16_t CELL_AT{ 0xee84 };

// The computer's targets, in percent of the capacity: low, full, and what it adds to its side's fill.
constexpr std::uint16_t LOW_TARGET{ 10 };
constexpr std::uint16_t FULL_TARGET{ 100 };
constexpr std::uint16_t FILL_MARGIN{ 30 };

// The scratch word n.
constexpr auto Scratch(unsigned n) -> std::uint16_t {
  return static_cast<std::uint16_t>(SCRATCH_AT + 2 * n);
}

// The near pointer of a transfer_table word.
constexpr auto TableAt(unsigned side, unsigned row, unsigned column) -> std::uint16_t {
  return static_cast<std::uint16_t>(offsetof(DataSegment, transfer_table) + 2 * ((side * 3u + row) * 3u + column));
}

// Whether the computer has taken over the regiment's division (it runs that division's transfers).
auto DivisionHeld(std::uint8_t* ds, HostWordsEvent event, std::uint16_t unit) -> bool {
  return Record<Group>(Data(ds), Call(ds, event, UNIT_GROUP, unit)).flags & Bits(GroupFlag::AI_MARK);
}

auto UnitName(std::uint8_t* ds, HostWordsEvent event, std::uint16_t unit, std::uint16_t buffer) -> std::uint16_t {
  return Call(ds, event, UNIT_NAME, unit, buffer);
}

// Writes the table's amounts back to the regiment.
void StoreUnit(DataSegment& data, std::uint16_t unit, std::uint8_t side) {
  auto& regiment{ Record<Unit>(data, unit) };
  for (std::uint8_t resource = 0; resource < RESOURCES; ++resource)
    regiment.resources[resource] = data.transfer_table[side][resource][AMOUNT];
}

// The supply order next to the current regiment, of an objective or of another regiment.
auto SupplyOrder(std::uint8_t* ds, HostWordsEvent event, bool objective) -> std::uint16_t {
  auto&      data{ Data(ds) };
  auto const target{ objective ? Call(ds, event, OBJECTIVE_TARGET, CELL_AT, CELL_AT + 1, data.current_side)
                               : Call(ds, event, UNIT_TARGET, CELL_AT, CELL_AT + 1) };
  if (target) {
    if (!Call(ds, event, ADJACENT, Byte(ds, CELL_AT), Byte(ds, CELL_AT + 1))) return 0;
    auto const hours{ opeu_transfer_level(ds, event) };
    // The order takes its hours before the supplies move.
    if (!Call(ds, event, ORDER_PROGRESS, Near(data, Record<Unit>(data, data.current_unit_ptr).orders[0]), hours))
      return 1;
    auto const unit{ data.current_unit_ptr };
    if (objective)
      opeu_transfer_objective(ds, event, unit, target);
    else
      opeu_transfer_units(ds, event, unit, target);
  }
  Call(ds, event, RESTORE_ORDER, data.current_unit_ptr);
  return 1;
}
} // namespace

// Loads a regiment into the table's `side`: its amounts, its capacities and the computer's targets in percent of
// each capacity. The division's eighth regiment fills to 10%, or beside an objective to 100% of a resource the
// objective holds above its reserve; other regiments fill to their side's percentage plus 30, at most 100.
void opeu_transfer_load_unit(uint8_t ds[65536], HostWordsEvent event, uint16_t unit, uint8_t side) {
  auto&       data{ Data(ds) };
  auto const& regiment{ Record<Unit>(data, unit) };
  opeu_transfer_set(ds, side, 0, AMOUNT, regiment.resources[0]);
  opeu_transfer_set(ds, side, 1, AMOUNT, regiment.resources[1]);
  opeu_transfer_set(ds, side, 2, AMOUNT, regiment.resources[2]);
  std::array<std::uint16_t, RESOURCES> const words{ Scratch(1), Scratch(5), Scratch(3) };
  Call(ds, event, UNIT_CAPACITIES, unit, words[0], words[1], words[2]);
  std::array<std::uint16_t, RESOURCES> const capacity{ Word(ds, words[0]), Word(ds, words[1]), Word(ds, words[2]) };
  for (std::uint8_t resource = 0; resource < RESOURCES; ++resource)
    opeu_transfer_set(ds, side, resource, CAPACITY, capacity[resource]);
  std::array<std::uint16_t, RESOURCES> target;
  if (Call(ds, event, LAST_SLOT, static_cast<std::uint8_t>(Call(ds, event, UNIT_SLOT, unit)))) {
    if (!data.transfer_objective_ptr) {
      target.fill(LOW_TARGET);
    } else {
      auto const& objective{ Record<Objective>(data, data.transfer_objective_ptr) };
      for (std::uint8_t resource = 0; resource < RESOURCES; ++resource)
        target[resource] = objective.resources[resource] > objective.reserves[resource] ? FULL_TARGET : LOW_TARGET;
    }
  } else {
    std::array<std::uint16_t, RESOURCES> const fill{ Scratch(0), Scratch(4), Scratch(2) };
    Call(ds, event, SIDE_PERCENTAGES, data.current_side, fill[0], fill[1], fill[2], 0);
    for (std::uint8_t resource = 0; resource < RESOURCES; ++resource)
      target[resource] = opeu_min16(static_cast<std::uint16_t>(Word(ds, fill[resource]) + FILL_MARGIN), FULL_TARGET);
  }
  for (std::uint8_t resource = 0; resource < RESOURCES; ++resource)
    opeu_transfer_set(ds, side, resource, TARGET, opeu_percent(capacity[resource], target[resource]));
}

// Loads an objective into the table's right side: its capacities, its reserves as the targets and its amounts.
void opeu_transfer_load_objective(uint8_t ds[65536], HostWordsEvent event, uint16_t objective) {
  auto& data{ Data(ds) };
  Call(ds, event, OBJECTIVE_CAPACITIES, objective, TableAt(1, 0, CAPACITY), TableAt(1, 1, CAPACITY),
       TableAt(1, 2, CAPACITY));
  auto const& city{ Record<Objective>(data, objective) };
  for (std::uint8_t resource = 0; resource < RESOURCES; ++resource)
    data.transfer_table[1][resource][TARGET] = city.reserves[resource];
  for (std::uint8_t resource = 0; resource < RESOURCES; ++resource)
    data.transfer_table[1][resource][AMOUNT] = city.resources[resource];
}

// The transfer between two regiments, repeated until it is confirmed (or the computer finishes); the player runs it
// when a player commands the side, unless the computer has taken over both divisions.
void opeu_transfer_units(uint8_t ds[65536], HostWordsEvent event, uint16_t first, uint16_t second) {
  auto& data{ Data(ds) };
  data.transfer_objective_ptr = 0;
  do {
    opeu_transfer_load_unit(ds, event, first, 0);
    opeu_transfer_load_unit(ds, event, second, 1);
    // The second name is taken first (the original's right-to-left arguments).
    auto const second_name{ UnitName(ds, event, second, SECOND_NAME_AT) };
    auto const first_name{ UnitName(ds, event, first, FIRST_NAME_AT) };
    opeu_transfer_draw(ds, event, first_name, second_name);
  } while (!opeu_transfer_run(ds, event,
                              data.side_control[data.current_side] == SIDE_PLAYED
                                  && (!DivisionHeld(ds, event, first) || !DivisionHeld(ds, event, second))));
  StoreUnit(data, first, 0);
  StoreUnit(data, second, 1);
}

// The transfer between a regiment and an objective.
void opeu_transfer_objective(uint8_t ds[65536], HostWordsEvent event, uint16_t unit, uint16_t objective) {
  auto& data{ Data(ds) };
  data.transfer_objective_ptr = objective;
  do {
    opeu_transfer_load_unit(ds, event, unit, 0);
    opeu_transfer_load_objective(ds, event, objective);
    opeu_transfer_draw(ds, event, UnitName(ds, event, unit, FIRST_NAME_AT), objective);
  } while (!opeu_transfer_run(ds, event,
                              data.side_control[data.current_side] == SIDE_PLAYED && !DivisionHeld(ds, event, unit)));
  StoreUnit(data, unit, 0);
  auto& city{ Record<Objective>(data, objective) };
  for (std::uint8_t resource = 0; resource < RESOURCES; ++resource)
    city.resources[resource] = data.transfer_table[1][resource][AMOUNT];
  Call(ds, event, DAILY_CONTEXT, data.current_side);
}

// The hours a supply order of the current regiment takes: 4, plus its experience / 24, less its training / 32.
uint8_t opeu_transfer_level(uint8_t ds[65536], HostWordsEvent event) {
  auto const unit{ Data(ds).current_unit_ptr };
  auto const training{ static_cast<std::uint16_t>(Call(ds, event, UNIT_TRAINING, unit) >> 5) };
  return static_cast<std::uint8_t>(Call(ds, event, UNIT_EXPERIENCE, unit) / 24 - training + 4);
}

uint16_t opeu_order_supply_objective(uint8_t ds[65536], HostWordsEvent event) {
  return SupplyOrder(ds, event, true);
}

uint16_t opeu_order_supply_unit(uint8_t ds[65536], HostWordsEvent event) {
  return SupplyOrder(ds, event, false);
}

uint16_t svc_xferunit(uint8_t* ds, uint16_t id, uint16_t* args, uint16_t count) {
  return svc_transfer(ds, "xferunit", id, args, count);
}

auto port_routine_opeu_order_supply_objective(std::uint8_t* ds, std::uint16_t const* args, unsigned count)
    -> std::uint32_t {
  (void)args;
  (void)count;
  return opeu_order_supply_objective(ds, svc_xferunit);
}

auto port_routine_opeu_order_supply_unit(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return opeu_order_supply_unit(ds, svc_xferunit);
}
