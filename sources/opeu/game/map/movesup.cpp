// Fuel for the current unit's move: whether it can move at all (not when one of its element types uses fuel and the
// unit has none), and the fuel the move uses up.
#include "map/movesup.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/game.hpp"
#include "core/limits.hpp"
#include "core/numeric.hpp"
#include "types.hpp"

using namespace opeu::game;

namespace {
constexpr unsigned FUEL{ 1 }; // Unit::resources: ammunition, fuel, food

// The fuel an element uses for a move: scale * 1000 / min(usage, scale * 1000) * quantity / 100, each step truncated
// to 16 bits.
[[nodiscard]] auto FuelUsed(DataSegment const& data, Element const& element) -> uint16_t {
  auto const scale{ static_cast<uint16_t>(data.map_scale * 1000u) };
  auto const usage{ data.element_types[element.type].fuel_economy };
  auto const share{ static_cast<uint16_t>(scale / opeu_min16(usage, scale)) };
  return static_cast<uint16_t>(static_cast<uint16_t>(static_cast<uint32_t>(share) * element.quantity) / 100);
}
} // namespace

uint16_t opeu_move_supplied(Game& game) {
  auto const& data{ Data(game) };
  auto const& unit{ Record<Unit>(data, data.current_unit_ptr) };
  for (auto const& element : Elements(data, unit))
    if (data.element_types[element.type].fuel_economy) return unit.resources[FUEL] != 0;
  return 1;
}

// Each fuel-using element takes at least 1 from the unit's fuel, which stops at 0.
void opeu_move_consume(Game& game) {
  auto& data{ Data(game) };
  auto& unit{ Record<Unit>(data, data.current_unit_ptr) };
  for (auto const& element : Elements(data, unit)) {
    if (!data.element_types[element.type].fuel_economy) continue;
    uint16_t fuel{ unit.resources[FUEL] };
    opeu_sub_word(opeu_max16(FuelUsed(data, element), 1), &fuel);
    unit.resources[FUEL] = fuel;
  }
}
