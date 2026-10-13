// The supply exchange: loads the current and capacity resource words of two units, or of a unit and an objective,
// into the transfer table, runs the transfer dialog until it reports completion, then stores the edited amounts back.
#include "units/supply.hpp"
#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "core/suppanel.hpp"
#include "core/textout.hpp"
#include "types.hpp"
#include "units/capacity.hpp"
#include "units/resagg.hpp"
#include "units/unitname.hpp"

#include "common/game.hpp"
#include "core/supstep.hpp"
#include "port.hpp"
#include "services.hpp"

#include <algorithm>
#include <array>
#include <ranges>

using namespace opeu::game;

namespace {
constexpr std::uint16_t NO_OBJECTIVE{ 0 };
constexpr std::uint8_t  CURRENT{ 0 };
constexpr std::uint8_t  CAPACITY{ 1 };

// Draws the dialog for two units; a and b are the name buffers. The second name is formatted first.
auto DrawUnits(Game& game, std::uint16_t first, std::uint16_t a, std::uint16_t second, std::uint16_t b) -> void {
  auto const second_name{ opeu_unit_name(game, second, b) };
  opeu_supply_draw(game, opeu_unit_name(game, first, a), second_name);
}

// A unit name buffer the unit-name service formats into.
constexpr std::uint16_t NAME_BYTES{ 0x20 };

// Runs the dialog until it reports completion.
auto RunDialog(Game& game) -> void {
  while (!opeu_supply_step(game)) {
  }
}

// Stores the edited current amounts of a party back into the resources of its record (a Unit or an Objective).
auto StoreBack(DataSegment const& data, std::uint8_t party, auto& record) -> void {
  std::ranges::transform(data.supply_table[party], record.resources, [](auto const& row) { return row[CURRENT]; });
}

// The resource rows of the supply table, ammunition, fuel and food.
constexpr auto RESOURCE_ROWS{ std::views::iota(std::uint8_t{ 0 }, std::uint8_t{ 3 }) };
} // namespace

void opeu_supply_load_unit(Game& game, uint16_t unit, uint8_t side) {
  auto const& record{ Record<Unit>(Data(game), unit) };
  for (auto const [resource, amount] : std::views::zip(RESOURCE_ROWS, record.resources)) opeu_supply_set(game, side, resource, CURRENT, amount);
  for (auto const [resource, capacity] : std::views::zip(RESOURCE_ROWS, Figures(UnitCapacities(Data(game), unit)))) opeu_supply_set(game, side, resource, CAPACITY, capacity);
}

void opeu_supply_load_objective(Game& game, uint16_t objective) {
  auto&       data{ Data(game) };
  auto const& record{ Record<Objective>(data, objective) };
  for (auto&& [row, capacity, amount] : std::views::zip(data.supply_table[1], Figures(ObjectiveCapacities(data, objective)), record.resources)) {
    row[CAPACITY] = capacity;
    row[CURRENT]  = amount;
  }
}

// Exchanges supplies between two units; the units' names are formatted into the routine's two 32-byte buffers.
void opeu_supply_exchange_units(Game& game, uint16_t first, uint16_t second) {
  auto&               data{ Data(game) };
  ServiceLocals const names{ 2 * NAME_BYTES };
  data.supply_objective = NO_OBJECTIVE;
  opeu_supply_load_unit(game, first, 0);
  opeu_supply_load_unit(game, second, 1);
  DrawUnits(game, first, names.At(NAME_BYTES), second, names.At(0));
  RunDialog(game);
  StoreBack(data, 0, Record<Unit>(data, first));
  StoreBack(data, 1, Record<Unit>(data, second));
}

// Exchanges supplies between a unit and an objective; the unit's name is formatted into the routine's 32-byte buffer.
void opeu_supply_exchange_objective(Game& game, uint16_t unit, uint16_t objective) {
  auto&               data{ Data(game) };
  ServiceLocals const name{ NAME_BYTES };
  data.supply_objective = objective;
  opeu_supply_load_unit(game, unit, 0);
  opeu_supply_load_objective(game, objective);
  opeu_supply_draw(game, opeu_unit_name(game, unit, name.At(0)), objective);
  RunDialog(game);
  StoreBack(data, 0, Record<Unit>(data, unit));
  StoreBack(data, 1, Record<Objective>(data, objective));
}
