// The computer side's supply budget: how many 3000-unit loads of a resource it takes from its stock.
#include "core/aibudget.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "core/limits.hpp"

using namespace opeu::game;

namespace {

constexpr std::uint16_t LOAD{ 3000 };    // the units of one supply load
constexpr std::uint16_t MAX_LOADS{ 20 }; // the most loads taken at once
} // namespace

// Takes whole loads of resource `which` (0 ammunition, 1 fuel, 2 food) from the current side's stock: at most 20, at
// most *budget and at most amount / 3000. Subtracts the count from *budget and returns the stock rounded down to a
// whole number of loads (0 for any other `which`).
uint16_t opeu_ai_supply_share(Game& game, uint16_t* budget, uint16_t amount, uint8_t which) {
  auto&         data{ Data(game) };
  std::uint16_t stock{ which < 3 ? Record<Side>(data, data.current_side_ptr).stock[which] : std::uint16_t{ 0 } };
  stock   /= LOAD;
  *budget -= opeu_min16(static_cast<std::uint16_t>(amount / LOAD), opeu_min16(stock, opeu_min16(*budget, MAX_LOADS)));
  return static_cast<std::uint16_t>(stock * LOAD);
}
