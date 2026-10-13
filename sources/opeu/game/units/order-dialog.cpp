// The target check and the acknowledgement the two order dialogs share.
#include "units/order-dialog.hpp"
#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "common/order.hpp"
#include "core/recindex.hpp"
#include "core/sideutil.hpp"
#include "map/cellflag.hpp"
#include "map/grid.hpp"
#include "map/query.hpp"
#include "map/support.hpp"
#include "units/ordunit.hpp"
#include "units/stats.hpp"
#include "units/value.hpp"

using namespace opeu::game;

namespace {
// The texts message_lookup gives for an order.
enum OrderMessage : std::uint16_t {
  NO_MINES_THERE  = 0x6e, // mines cannot be laid on the cell
  BRIDGE_ORDERED  = 0x6f,
  CLEAR_ORDERED   = 0x70,
  DESTROY_ORDERED = 0x71,
};
// The two-choice menus and the answers of their dialog.
constexpr std::uint16_t SUPPLY_MENU{ 2 };
constexpr std::uint16_t REPAIR_MENU{ 3 };
constexpr std::uint16_t CHOICE_COUNT{ 2 };
constexpr std::uint16_t CHOOSE_CITY{ 1 };
constexpr std::uint16_t CHOOSE_REGIMENT{ 2 };

// Supply or repair at a cell: of an own city there, of an own regiment there, or neither. With both, the player is
// asked which.
enum class Choice {
  CITY,
  REGIMENT,
  NEITHER
};
auto ChooseCityOrRegiment(Game& game, HostEvent event, std::uint8_t side, std::uint8_t city_side, std::uint8_t unit_side, std::uint16_t menu, std::uint16_t which, std::uint16_t choices,
                          std::uint16_t person) -> Choice {
  if (side == city_side && side == unit_side) {
    opeu_order_layout_pair(game, menu);
    switch (Call(game, event, ORDERS_DELAY_DIALOG, which, CHOICE_COUNT, person, choices)) {
    case CHOOSE_CITY:     return Choice::CITY;
    case CHOOSE_REGIMENT: return Choice::REGIMENT;
    default:              return Choice::NEITHER;
    }
  }
  if (side == city_side) return Choice::CITY;
  if (side != unit_side) return Choice::NEITHER;
  return Choice::REGIMENT;
}
} // namespace

auto OrderMenuKind(std::uint8_t slot) -> std::uint8_t {
  if (opeu_category_6(slot)) return 1;
  if (opeu_category_7(slot)) return 2;
  return 0;
}

auto OrderCount(std::array<std::uint8_t, 8> const& codes) -> std::uint8_t {
  std::uint8_t count{ 0 };
  while (count < 8 && codes[count] != ORDER_UNUSED) ++count;
  return count;
}

auto CheckOrderTarget(Game& game, HostEvent event, OrderPrompts const& prompts, std::uint16_t buffer, std::uint16_t person, std::uint16_t unit, std::uint8_t x, std::uint8_t y, std::uint8_t& kind,
                      std::uint16_t& target) -> bool {
  auto&      data{ Data(game) };
  auto const side{ data.current_side };
  auto const enemy{ opeu_opposite_side(side) };
  auto const objective{ opeu_find_objective_at(game, x, y) };
  auto const city_side{ objective ? opeu_objective_side(game, objective) : NO_SIDE };
  target = opeu_find_unit_at(game, x, y);
  std::uint8_t unit_side{ target ? opeu_unit_side(target) : NO_SIDE };
  // The regiment itself (except for a repair), an unsighted enemy and an own regiment that may not be selected do
  // not count as targets.
  if ((kind != Kind(UnitOrder::REPAIR_CITY) && target == unit) || (enemy == unit_side && !opeu_unit_special(game, target)) || (side == unit_side && !opeu_unit_selection_eligible(game, target))) {
    target    = 0;
    unit_side = NO_SIDE;
  }
  auto const place{ [&] {
    target = 0;
    return true;
  } };
  auto const refuse{ [&](auto const& text) {
    Call(game, event, ORDERS_REQUEST_FORMAT, buffer, Near(data, text));
    return false;
  } };
  auto const cannot{ [&] {
    Call(game, event, ORDERS_REQUEST_FORMAT, buffer, Near(data, prompts.cannot_order), data.order_names[kind + 3]);
    return false;
  } };
  auto const terrain{ [&] { return static_cast<Terrain>(opeu_map_code(game, x, y)); } };
  switch (static_cast<UnitOrder>(kind)) {
  case UnitOrder::MOVE:        return terrain() != Terrain::IMPASSABLE ? place() : cannot();
  case UnitOrder::OCCUPY:      return enemy == city_side ? place() : refuse(prompts.not_enemy_city);
  case UnitOrder::REPAIR_CITY:
  case UnitOrder::RESUPPLY:    {
    auto const repair{ kind == Kind(UnitOrder::REPAIR_CITY) };
    switch (ChooseCityOrRegiment(game, event, side, city_side, unit_side, repair ? REPAIR_MENU : SUPPLY_MENU, repair ? Near(data, prompts.fix_which) : Near(data, prompts.supply_which),
                                 repair ? Near(data, data.repair_choices) : Near(data, data.supply_choices), person)) {
    case Choice::CITY:     return place();
    case Choice::NEITHER:  return cannot();
    case Choice::REGIMENT: kind = Kind(repair ? UnitOrder::REPAIR_UNIT : UnitOrder::SUPPLY_UNIT); return true;
    }
    return true;
  }
  case UnitOrder::DEFEND:
  case UnitOrder::REST:      return enemy == city_side ? cannot() : place();
  case UnitOrder::LAY_MINES: {
    if (opeu_cell_marked(game, x, y, side)) {
      kind = Kind(UnitOrder::CLEAR_MINES);
      return place();
    }
    switch (terrain()) {
    case Terrain::CITY:
    case Terrain::RIVER:
    case Terrain::BRIDGE:
    case Terrain::IMPASSABLE: Call(game, event, ORDERS_REQUEST_FORMAT, buffer, Call(game, event, ORDERS_MESSAGE_LOOKUP, NO_MINES_THERE)); return false;
    default:                  return place();
    }
  }
  case UnitOrder::BUILD_BRIDGE:   return terrain() == Terrain::RIVER ? place() : refuse(prompts.cannot_bridge);
  case UnitOrder::DESTROY_BRIDGE: return terrain() == Terrain::BRIDGE ? place() : refuse(prompts.no_bridge);
  case UnitOrder::ATTACK:         return enemy == unit_side ? true : refuse(prompts.no_enemy);
  case UnitOrder::PURSUE:         return side == unit_side ? true : refuse(prompts.nothing_to_chase);
  default:                        return true;
  }
}

auto AnnounceOrder(Game& game, HostEvent event, OrderPrompts const& prompts, std::uint16_t buffer, std::uint16_t person, std::uint8_t kind) -> void {
  auto& data{ Data(game) };
  Call(game, event, ORDERS_REQUEST_FORMAT, buffer, Near(data, prompts.understood), data.order_names[kind + 3]);
  auto const engineering{ [&](OrderMessage message) { Call(game, event, ORDERS_REQUEST_FORMAT, buffer, Call(game, event, ORDERS_MESSAGE_LOOKUP, message)); } };
  if (kind == Kind(UnitOrder::BUILD_BRIDGE)) engineering(BRIDGE_ORDERED);
  if (kind == Kind(UnitOrder::CLEAR_MINES)) engineering(CLEAR_ORDERED);
  if (kind == Kind(UnitOrder::DESTROY_BRIDGE)) engineering(DESTROY_ORDERED);
  Call(game, event, ORDERS_CONTROL_MESSAGE, buffer, person);
}
