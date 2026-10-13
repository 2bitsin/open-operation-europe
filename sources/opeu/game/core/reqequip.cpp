// Delivering a battalion request: equipment of the requested type added to an item slot of the objective.
#include "core/reqequip.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "core/dialogs.hpp"
#include "core/limits.hpp"
#include "core/msglook.hpp"
#include "core/numeric.hpp"
#include "core/reqcheck.hpp"
#include "core/reqres.hpp"
#include "core/textout.hpp"
#include "data-segment.hpp"
#include "io/soundcue.hpp"
#include "map/mappos.hpp"
#include "types.hpp"
#include <array>

using namespace opeu::game;

namespace {

constexpr uint16_t HEADER_MESSAGE{ 74 }; // the report's header message
constexpr uint16_t COMPUTER_BONUS{ 18 }; // added to the delivery of a side the computer commands

} // namespace

// Adds equipment of the requested type to the objective's slot for it: the capacity of the type's category divided
// by one plus a twentieth of its better rating, raised for the difficulty level (a person's side) or by a fixed bonus
// (the computer's), delivered as a half to five sixths of that. A person playing the side sees the report.
void opeu_request_action1(Game& game, uint8_t side, uint16_t request) {
  auto&       data{ Data(game) };
  auto const& slot{ Record<RequestSlot>(data, request) };
  auto&       objective{ Record<Objective>(data, slot.target) };
  auto&       item{ Record<Item>(data, opeu_request_slot(game, Near(data, objective), slot.argument)) };
  item.type = slot.argument;
  auto const& type{ data.element_types[item.type] };
  auto const  before{ item.quantity };
  if (!before) item.extra = 0;
  auto const capacity{ data.request_capacities[type.category] };
  auto       amount{ static_cast<uint16_t>(capacity / (opeu_max16(type.anti_tank, type.anti_personnel) / 20 + 1)) };
  if (data.side_control[side] == SIDE_PLAYED)
    amount = static_cast<uint16_t>(amount + (5 - data.delivery_level) * 6);
  else
    amount = static_cast<uint16_t>(amount + COMPUTER_BONUS);
  item.quantity = AddWord(item.quantity, game.State().RandomBounded(amount / 3) + amount / 2, capacity);
  if (data.side_status[side] != SIDE_PLAYED) return;
  auto const buffer{ static_cast<uint16_t>(offsetof(DataSegment, request_buffer)) };
  opeu_arrival_location(game, objective.x, objective.y, side);
  opeu_request_format(game, std::array<std::uint16_t, 3>{ buffer, opeu_message_lookup(game, HEADER_MESSAGE), Near(data, objective) });
  opeu_control_message(game, buffer, data.request_person);
  opeu_request_wait(game);
  opeu_request_format(game, std::array<std::uint16_t, 5>{ buffer, static_cast<std::uint16_t>(offsetof(DataSegment, equipment_delivery_format)), data.element_types[item.type].name, item.quantity,
                                                          static_cast<std::uint16_t>(item.quantity - before) });
  opeu_control_message(game, buffer, data.request_person);
}
