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

#include "core/supstep.hpp"
#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

using SupplyEvent = uint16_t (*)(uint8_t*, uint16_t, const uint16_t*, uint16_t);

namespace {
// The transfer dialog's services: 100 draws it with the two parties' names, 101 runs one step and reports completion.
enum : std::uint16_t {
  SUPPLY_DRAW = 100,
  SUPPLY_STEP = 101,
};

constexpr std::uint16_t NO_OBJECTIVE{ 0 };
constexpr std::uint8_t  CURRENT{ 0 };
constexpr std::uint8_t  CAPACITY{ 1 };

auto Raise(uint8_t* ds, SupplyEvent event, std::uint16_t id, std::uint16_t count, std::uint16_t first, std::uint16_t second) -> std::uint16_t {
  std::uint16_t args[2]{ first, second };
  return event(ds, id, args, count);
}

// Draws the dialog for two units; a and b are the name buffers. The second name is formatted first.
auto DrawUnits(uint8_t* ds, SupplyEvent event, std::uint16_t first, std::uint16_t a, std::uint16_t second, std::uint16_t b) -> void {
  auto const second_name{ opeu_unit_name(ds, event, second, b) };
  Raise(ds, event, SUPPLY_DRAW, 2, opeu_unit_name(ds, event, first, a), second_name);
}

// A unit name buffer the unit-name service formats into.
constexpr std::uint16_t NAME_BYTES{ 0x20 };

// Runs the dialog until it reports completion.
auto RunDialog(uint8_t* ds, SupplyEvent event) -> void {
  while (!Raise(ds, event, SUPPLY_STEP, 0, 0, 0)) {
  }
}

// Stores the edited current amounts of a party back into the resources of its record (a Unit or an Objective).
auto StoreBack(DataSegment& data, std::uint8_t party, auto& record) -> void {
  for (std::uint8_t resource{ 0 }; resource < 3; ++resource) record.resources[resource] = data.supply_table[party][resource][CURRENT];
}
} // namespace

void opeu_supply_load_unit(uint8_t ds[65536], uint16_t unit, uint8_t side) {
  auto const& record{ Record<Unit>(Data(ds), unit) };
  for (std::uint8_t resource{ 0 }; resource < 3; ++resource) opeu_supply_set(ds, side, resource, CURRENT, record.resources[resource]);
  uint16_t ammunition, fuel, food;
  opeu_unit_capacities(ds, unit, &ammunition, &fuel, &food);
  opeu_supply_set(ds, side, 0, CAPACITY, ammunition);
  opeu_supply_set(ds, side, 1, CAPACITY, fuel);
  opeu_supply_set(ds, side, 2, CAPACITY, food);
}

void opeu_supply_load_objective(uint8_t ds[65536], uint16_t objective) {
  auto&    data{ Data(ds) };
  uint16_t ammunition, fuel, food;
  opeu_objective_capacities(ds, objective, &ammunition, &fuel, &food);
  data.supply_table[1][0][CAPACITY] = ammunition;
  data.supply_table[1][1][CAPACITY] = fuel;
  data.supply_table[1][2][CAPACITY] = food;
  auto const& record{ Record<Objective>(data, objective) };
  for (std::uint8_t resource{ 0 }; resource < 3; ++resource) data.supply_table[1][resource][CURRENT] = record.resources[resource];
}

// Exchanges supplies between two units; the units' names are formatted into the routine's two 32-byte buffers.
void opeu_supply_exchange_units(uint8_t ds[65536], SupplyEvent event, uint16_t first, uint16_t second) {
  auto&               data{ Data(ds) };
  ServiceLocals const names{ 2 * NAME_BYTES };
  data.supply_objective = NO_OBJECTIVE;
  opeu_supply_load_unit(ds, first, 0);
  opeu_supply_load_unit(ds, second, 1);
  DrawUnits(ds, event, first, names.At(NAME_BYTES), second, names.At(0));
  RunDialog(ds, event);
  StoreBack(data, 0, Record<Unit>(data, first));
  StoreBack(data, 1, Record<Unit>(data, second));
}

// Exchanges supplies between a unit and an objective; the unit's name is formatted into the routine's 32-byte buffer.
void opeu_supply_exchange_objective(uint8_t ds[65536], SupplyEvent event, uint16_t unit, uint16_t objective) {
  auto&               data{ Data(ds) };
  ServiceLocals const name{ NAME_BYTES };
  data.supply_objective = objective;
  opeu_supply_load_unit(ds, unit, 0);
  opeu_supply_load_objective(ds, objective);
  Raise(ds, event, SUPPLY_DRAW, 2, opeu_unit_name(ds, event, unit, name.At(0)), objective);
  RunDialog(ds, event);
  StoreBack(data, 0, Record<Unit>(data, unit));
  StoreBack(data, 1, Record<Objective>(data, objective));
}

// The services the module calls out to: the transfer dialog's draw and step, and request_format (id 2), which
// opeu_unit_name raises through the event it receives.
uint16_t svc_supply(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r = 0;
  switch (id) {
  case 2:           opeu_request_format(ds, { args, count }); break;
  case SUPPLY_DRAW: opeu_supply_draw(ds, svc_suppanel, Arg(args, count, 0), Arg(args, count, 1)); break;
  case SUPPLY_STEP: r = opeu_supply_step(ds, svc_supstep); break;
  default:          port_unhandled("supply", id, count);
  }
  return r;
}
