// The engineering orders: laying and clearing mines, building a bridge and repairing the arms of an objective or a
// regiment. Each handler takes the current regiment's order destination and needs the regiment next to it; mines and
// bridges lower the order's progress by a skill roll (the general's leadership, the regiment's averages, its engineers
// and a random term) and, when the progress runs out, report, change the map cell and show the day's picture whose
// finish routine ends the pending wait. A repair moves damaged arms back into service, up to the type's capacity, with
// engineers' strength / 10 + 1 per turn.
#include "units/engineer.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "core/limits.hpp"
#include "core/textout.hpp"
#include "types.hpp"
#include "units/order-handler.hpp"

#include "common/game.hpp"
#include "core/dialogs.hpp"
#include "core/objrank.hpp"
#include "core/recindex.hpp"
#include "core/sideutil.hpp"
#include "io/soundcue.hpp"
#include "io/waitkey.hpp"
#include "link/glue.hpp"
#include "map/cellflag.hpp"
#include "map/grid.hpp"
#include "map/maperase.hpp"
#include "map/movesup.hpp"
#include "map/movetgt.hpp"
#include "map/moveunit.hpp"
#include "map/occupy.hpp"
#include "map/query.hpp"
#include "map/stepmap.hpp"
#include "map/support.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/orders.hpp"
#include "units/stats.hpp"
#include "units/unitname.hpp"
#include "units/unitpers.hpp"
#include "units/value.hpp"

#include <utility>

using namespace opeu::game;

namespace {

// The pictures of the day's show.
enum Picture : std::uint16_t {
  MINES_LAID_PICTURE    = 0x191,
  MINES_REMOVED_PICTURE = 0x68,
  BRIDGE_PICTURE        = 0x1a2,
  ARMS_PICTURE          = 0x1b4,
};

// The engineering skill roll, computed in 16-bit words of which the low byte is kept. The services are called in the
// order general, random bound, engineer count, average10, average11.
auto SkillRoll(Game& game, std::uint16_t person, std::uint16_t bound, std::uint16_t leadership_divisor, std::uint16_t engineer_divisor, std::uint16_t bonus) -> std::uint8_t {
  auto&               data{ Data(game) };
  std::uint16_t const random{ game.State().RandomBounded(bound) };
  std::uint16_t const engineers{ UnitQuantity(data, data.current_unit_ptr, Bits(Category::ENGINEERS)).total };
  std::uint16_t const average10{ opeu_unit_average10(game, data.current_unit_ptr) };
  std::uint16_t const average11{ opeu_unit_average11(game, data.current_unit_ptr) };
  return static_cast<std::uint8_t>(average11 / 19 - static_cast<std::uint16_t>(Record<Person>(data, person).leadership + average10) / leadership_divisor - engineers / engineer_divisor + random
                                   + bonus);
}

// Lowers the current order's progress by the roll; nonzero when it runs out. The regiment is read after the roll.
auto LowerProgress(Game& game, std::uint8_t roll) -> std::uint16_t {
  auto& data{ Data(game) };
  return opeu_move_level_lower(game, Near(data, Record<Unit>(data, data.current_unit_ptr).orders[0]), static_cast<uint8_t>(roll));
}

auto Show(Game& game, Picture picture, Presenter finish) -> void {
  opeu_daily_show(game, picture, static_cast<uint8_t>(Data(game).current_side), finish);
}

auto Report(Game& game, auto const& text) -> void {
  auto& data{ Data(game) };
  opeu_unit_report(game, data.current_unit_ptr, Near(data, text));
}

// The strength of the current regiment's repair crews.
[[nodiscard]] auto RepairCrews(DataSegment const& data) -> std::uint16_t {
  return UnitQuantity(data, data.current_unit_ptr, Bits(Category::REPAIR)).total;
}

auto RestoreOrder(Game& game) -> void {
  opeu_restore_unit_order(game, Data(game).current_unit_ptr);
}
} // namespace

uint8_t opeu_mine_skill(Game& game) {
  auto const person{ opeu_unit_person(game, Data(game).current_unit_ptr) };
  return SkillRoll(game, person, RollBound(Data(game)), 45, 96, 8);
}

void opeu_mines_laid_finish(Game& game) {
  opeu_pending_finish(game, static_cast<uint8_t>(SHOW_DONE));
}

// Lays mines at the destination cell unless the side has mined it already or the regiment stands on it.
uint16_t opeu_engineer_lay_mines(Game& game) {
  auto& data{ Data(game) };
  auto const [x, y]{ DestinationCell(game) };
  auto const& regiment{ Record<Unit>(data, data.current_unit_ptr) };
  if (!opeu_cell_marked(game, x, y, static_cast<uint8_t>(data.current_side)) && (regiment.x != x || regiment.y != y)) {
    if (!opeu_move_adjacent(game, x, y)) return 0;
    if (LowerProgress(game, opeu_mine_skill(game)) && opeu_cell_side(game, x, y) == NO_SIDE) {
      Report(game, data.mines_laid_text);
      opeu_mark_cell(game, x, y, static_cast<uint8_t>(data.current_side));
      opeu_map_erase(game, x, y);
      Show(game, MINES_LAID_PICTURE, Presenter::MINES_LAID);
      RestoreOrder(game);
    }
    return 1;
  }
  RestoreOrder(game);
  return 1;
}

void opeu_mines_removed_finish(Game& game) {
  opeu_pending_finish(game, static_cast<uint8_t>(SHOW_DONE));
}

// Clears the mines of both sides at the destination cell.
uint16_t opeu_engineer_remove_mines(Game& game) {
  auto& data{ Data(game) };
  auto const [x, y]{ DestinationCell(game) };
  if (opeu_cell_marked(game, x, y, static_cast<uint8_t>(data.current_side))) {
    if (!opeu_move_adjacent(game, x, y)) return 0;
    if (LowerProgress(game, opeu_mine_skill(game))) {
      Report(game, data.mines_removed_text);
      opeu_unmark_cell(game, x, y, 0);
      opeu_unmark_cell(game, x, y, 1);
      opeu_map_erase(game, x, y);
      Show(game, MINES_REMOVED_PICTURE, Presenter::MINES_REMOVED);
      RestoreOrder(game);
    }
    return 1;
  }
  RestoreOrder(game);
  return 1;
}

uint8_t opeu_bridge_skill(Game& game) {
  auto const person{ opeu_unit_person(game, Data(game).current_unit_ptr) };
  return SkillRoll(game, person, RollBound(Data(game)), 35, 80, 10);
}

void opeu_bridge_built_finish(Game& game) {
  opeu_pending_finish(game, static_cast<uint8_t>(SHOW_DONE));
}

// Builds a bridge over the river at the destination cell.
uint16_t opeu_engineer_build_bridge(Game& game) {
  auto& data{ Data(game) };
  auto const [x, y]{ DestinationCell(game) };
  if (static_cast<Terrain>(opeu_map_code(game, x, y)) == Terrain::RIVER) {
    if (!opeu_move_adjacent(game, x, y)) return 0;
    if (LowerProgress(game, opeu_bridge_skill(game)) && opeu_cell_side(game, x, y) == NO_SIDE) {
      Report(game, data.bridge_built_text);
      opeu_map_mark_10(game, x, y);
      opeu_map_erase(game, x, y);
      Show(game, BRIDGE_PICTURE, Presenter::BRIDGE_BUILT);
      RestoreOrder(game);
    }
    return 1;
  }
  RestoreOrder(game);
  return 1;
}

// Moves damaged arms of the item (an objective's item or an element's type, quantity and damage) back into service:
// at most the budget (lowered by what is repaired), the damaged count, and what the type's capacity leaves room for. Nonzero when the
// item had damage and budget left.
uint16_t opeu_arms_repair(Game& game, uint16_t& budget, uint16_t body) {
  auto& data{ Data(game) };
  auto& item{ Record<Item>(data, body) };
  if (!budget || !item.extra) return 0;
  auto const capacity{ data.request_capacities[data.element_types[item.type].category] };
  auto const repaired{ static_cast<std::uint16_t>(opeu_min16(opeu_min16(budget, item.extra) + item.quantity, capacity) - item.quantity) };
  item.extra    -= repaired;
  item.quantity += repaired;
  budget         = static_cast<std::uint16_t>(budget - repaired);
  return 1;
}

void opeu_arms_repaired_finish(Game& game) {
  opeu_pending_finish(game, static_cast<uint8_t>(SHOW_DONE));
}

// Repairs the arms of the own objective at the destination; the order ends when nothing is left to repair.
uint16_t opeu_engineer_repair_objective(Game& game) {
  auto& data{ Data(game) };
  if (!RepairCrews(data)) {
    RestoreOrder(game);
    return 1;
  }
  ServiceCell const target;
  auto const        at_x{ target.XAt() };
  auto const        at_y{ target.YAt() };
  auto const        objective{ opeu_move_objective_target(game, at_x, at_y, static_cast<uint8_t>(data.current_side)) };
  if (!objective) return 1;
  if (!opeu_move_adjacent(game, static_cast<uint8_t>(Byte(game, at_x)), static_cast<uint8_t>(Byte(game, at_y)))) return 0;
  Record<Unit>(data, data.current_unit_ptr).orders[0].progress = 0;
  auto budget{ static_cast<std::uint16_t>(RepairCrews(data) / 10 + 1) };
  bool repaired{ false };
  for (auto& item : Record<Objective>(data, objective).items)
    if (item.quantity && opeu_arms_repair(game, budget, Near(data, item))) repaired = true;
  if (!repaired) {
    Report(game, data.arms_repaired_text);
    Show(game, ARMS_PICTURE, Presenter::ARMS_REPAIRED);
    RestoreOrder(game);
  }
  return 1;
}

// Repairs the arms of the friendly regiment the order targets; the order ends when nothing is left to repair.
uint16_t opeu_engineer_repair_unit(Game& game) {
  auto& data{ Data(game) };
  if (!RepairCrews(data)) {
    RestoreOrder(game);
    return 1;
  }
  ServiceCell const target;
  auto const        at_x{ target.XAt() };
  auto const        at_y{ target.YAt() };
  auto const        unit{ opeu_move_unit_target(game, at_x, at_y) };
  if (!unit) return 1;
  if (!opeu_move_adjacent(game, static_cast<uint8_t>(Byte(game, at_x)), static_cast<uint8_t>(Byte(game, at_y)))) return 0;
  Record<Unit>(data, data.current_unit_ptr).orders[0].progress = 0;
  auto budget{ static_cast<std::uint16_t>(RepairCrews(data) / 10 + 1) };
  bool repaired{ false };
  for (auto element{ Record<Unit>(data, unit).first }; element; element = Record<Element>(data, element).next)
    if (opeu_arms_repair(game, budget, Near(data, Record<Element>(data, element).type))) repaired = true;
  if (!repaired) {
    Report(game, data.unit_arms_repaired_text);
    Show(game, ARMS_PICTURE, Presenter::ARMS_REPAIRED);
    RestoreOrder(game);
  }
  return 1;
}
