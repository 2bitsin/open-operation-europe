// What the two order dialogs share (units/ordissue, the Orders command; units/ordact, the map cursor's unit action):
// the services they call, checking a picked target cell against the order and announcing the accepted order.
#pragma once

#include "common/data-segment.hpp"
#include "common/events.hpp"

#include <array>
#include <cstdint>

// What ORDERS_PROMPT answers when the menu is cancelled.
inline constexpr std::uint16_t MENU_CANCEL{ 0xffff };

// The order menu a regiment gets: 1 for engineers (category 6 slots), 2 for supply and repair (category 7), else 0.
[[nodiscard]] auto OrderMenuKind(std::uint8_t slot) -> std::uint8_t;

// The number of orders in a menu's code row (up to the first 0xff, at most 8).
[[nodiscard]] auto OrderCount(std::array<std::uint8_t, 8> const& codes) -> std::uint8_t;

// Checks the picked cell (x, y) against the order `kind` of the regiment `unit` under the general `person`. Supply
// and repair of a cell holding both an own city and an own regiment ask which one is meant (and may switch `kind` to
// the regiment order). On success `target` is the target regiment (0 for orders on a cell) and the result is true;
// otherwise the reason is formatted into the text at `buffer`.
[[nodiscard]] auto CheckOrderTarget(Game& game, opeu::game::OrderPrompts const& prompts, std::uint16_t buffer, std::uint16_t person, std::uint16_t unit, std::uint8_t x, std::uint8_t y,
                                    std::uint8_t& kind, std::uint16_t& target) -> bool;

// Formats and shows the acknowledgement of an accepted order of `kind`.
auto AnnounceOrder(Game& game, opeu::game::OrderPrompts const& prompts, std::uint16_t buffer, std::uint16_t person, std::uint8_t kind) -> void;
