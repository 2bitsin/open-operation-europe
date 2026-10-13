// What the order handlers (units/ordarriv, ordattk, orddefnd, ordbridg, engineer) share: their service calls, the
// current regiment and its order, and the random bound of their rolls.
#pragma once

#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/order-service.hpp"

#include <cstdint>
#include <utility>

namespace opeu::game::detail::order_handler {

// pending_finish's value when a show ends.
inline constexpr std::uint16_t SHOW_DONE{ 3 };

// The near pointer of the running order (the current regiment's first order slot).
[[nodiscard]] inline auto CurrentOrder(DataSegment& data) noexcept -> std::uint16_t {
  return Near(data, CurrentUnit(data).orders[0]);
}

// The random bound of a handler's roll: the delivery level + 1 for a player's regiment, 3 for the computer's.
[[nodiscard]] inline auto RollBound(DataSegment const& data) noexcept -> std::uint16_t {
  return data.side_control[data.current_side] == SIDE_PLAYED ? static_cast<std::uint16_t>(data.delivery_level + 1) : 3;
}

// A cell a service writes into the data segment as two bytes (an order's destination or target): the handler hands
// their near addresses to the service and reads the bytes back.
class ServiceCell {
public:
  [[nodiscard]] auto XAt() const noexcept -> std::uint16_t {
    return _locals.At(0);
  }
  [[nodiscard]] auto YAt() const noexcept -> std::uint16_t {
    return _locals.At(2);
  }

private:
  ServiceLocals _locals{ 4 };
};

// Whether the current regiment stands on the cell whose coordinates are the DS bytes at x and y.
[[nodiscard]] inline auto StandsOn(std::uint8_t const* ds, std::uint16_t x, std::uint16_t y) noexcept -> bool {
  auto const& unit{ Record<Unit>(Data(ds), Data(ds).current_unit_ptr) };
  return unit.x == Record<std::uint8_t>(Data(ds), x) && unit.y == Record<std::uint8_t>(Data(ds), y);
}
} // namespace opeu::game::detail::order_handler

namespace opeu::game {
using detail::order_handler::CurrentOrder;
using detail::order_handler::RollBound;
using detail::order_handler::ServiceCell;
using detail::order_handler::SHOW_DONE;
using detail::order_handler::StandsOn;
} // namespace opeu::game
