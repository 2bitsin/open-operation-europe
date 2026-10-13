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
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

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
auto StoreBack(DataSegment& data, std::uint8_t party, auto& record) -> void {
  for (std::uint8_t resource{ 0 }; resource < 3; ++resource) record.resources[resource] = data.supply_table[party][resource][CURRENT];
}
} // namespace

void opeu_supply_load_unit(Game& game, uint16_t unit, uint8_t side) {
  auto const& record{ Record<Unit>(Data(game), unit) };
  for (std::uint8_t resource{ 0 }; resource < 3; ++resource) opeu_supply_set(game, side, resource, CURRENT, record.resources[resource]);
  uint16_t ammunition, fuel, food;
  opeu_unit_capacities(game, unit, &ammunition, &fuel, &food);
  opeu_supply_set(game, side, 0, CAPACITY, ammunition);
  opeu_supply_set(game, side, 1, CAPACITY, fuel);
  opeu_supply_set(game, side, 2, CAPACITY, food);
}

void opeu_supply_load_objective(Game& game, uint16_t objective) {
  auto&    data{ Data(game) };
  uint16_t ammunition, fuel, food;
  opeu_objective_capacities(game, objective, &ammunition, &fuel, &food);
  data.supply_table[1][0][CAPACITY] = ammunition;
  data.supply_table[1][1][CAPACITY] = fuel;
  data.supply_table[1][2][CAPACITY] = food;
  auto const& record{ Record<Objective>(data, objective) };
  for (std::uint8_t resource{ 0 }; resource < 3; ++resource) data.supply_table[1][resource][CURRENT] = record.resources[resource];
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
