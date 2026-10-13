// Element averages and quantity totals of units, divisions, sides and objectives.
#pragma once

#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "types.hpp"

#include <cstdint>

namespace opeu::game::detail::stats {

// The category that selects every element or item.
inline constexpr std::uint8_t ANY_CATEGORY{ 255 };

// How much of a category a unit, a division or an objective holds: the quantity in a wrapping word and the number of
// elements or items it is spread over in a wrapping byte.
struct Quantity {
  std::uint16_t total;
  std::uint8_t  count;
};

// The same for a side, whose divisions sum into 32 bits.
struct WideQuantity {
  std::uint32_t total;
  std::uint8_t  count;
};

// The quantity of the unit's elements of `category` (ANY_CATEGORY: all).
[[nodiscard]] auto UnitQuantity(DataSegment const& data, std::uint16_t unit, std::uint8_t category) -> Quantity;

// The same over the ready units of a division.
[[nodiscard]] auto GroupQuantity(DataSegment const& data, std::uint16_t group, std::uint8_t category) -> Quantity;

// The same over the ready divisions of a side.
[[nodiscard]] auto SideQuantity(DataSegment const& data, std::uint8_t side, std::uint8_t category) -> WideQuantity;

// The quantity an objective stocks of `category` (ANY_CATEGORY: all).
[[nodiscard]] auto ObjectiveQuantity(DataSegment const& data, std::uint16_t objective, std::uint8_t category) -> Quantity;
} // namespace opeu::game::detail::stats

namespace opeu::game {
using detail::stats::ANY_CATEGORY;
using detail::stats::GroupQuantity;
using detail::stats::ObjectiveQuantity;
using detail::stats::Quantity;
using detail::stats::SideQuantity;
using detail::stats::UnitQuantity;
using detail::stats::WideQuantity;
} // namespace opeu::game

uint16_t opeu_unit_special(Game& game, uint16_t unit);
uint16_t opeu_unit_average9(Game& game, uint16_t unit);
uint16_t opeu_unit_average10(Game& game, uint16_t unit);
uint16_t opeu_unit_average11(Game& game, uint16_t unit);
uint16_t opeu_unit_quantity(Game& game, uint16_t unit, uint8_t category, uint8_t* count);
uint32_t opeu_side_quantity(Game& game, uint8_t side, uint8_t category, uint8_t* count);
uint16_t opeu_objective_quantity(Game& game, uint16_t objective, uint8_t category, uint8_t* count);
