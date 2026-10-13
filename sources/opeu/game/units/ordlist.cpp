// Orders command (turn action 1): the player picks divisions of the side in turn. A division the computer runs gets
// its objective tasks edited; otherwise the player orders the whole division at once, or picks its regiments one by
// one for the order menu.
#include "units/ordlist.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/forces.hpp"
#include "common/memory.hpp"
#include "core/dialogs.hpp"
#include "core/people.hpp"
#include "io/listdlg.hpp"
#include "map/mapunit.hpp"
#include "map/ordtgt.hpp"
#include "types.hpp"
#include "units/order-dialog.hpp"
#include "units/ordissue.hpp"
#include "units/ordunit.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/ordact.hpp"

#include <algorithm>
#include <iterator>

using namespace opeu::game;

namespace {
auto RunList(Game& game, ListWindow& list) -> std::uint16_t {
  return opeu_list_dialog(game, Near(Data(game), list), static_cast<uint8_t>(LIST_LIMIT), static_cast<uint8_t>(LIST_FLAGS));
}
} // namespace

// Lists the eight regiments of the current side's division `group` in the regiment rows.
uint8_t opeu_order_units_fill(Game& game, uint8_t group) {
  auto&      data{ Data(game) };
  auto const units{ GroupUnits(data, data.current_side, group) };
  std::ranges::transform(units, std::begin(data.row_units), [&](Unit const& unit) { return Near(data, unit); });
  return static_cast<std::uint8_t>(units.size());
}

// The Orders command.
void opeu_turn_action_1(Game& game) {
  auto& data{ Data(game) };
  opeu_title_panel_begin(game);
  data.unit_order_active = 0;
  for (;;) {
    SetListPredicate(data.division_dialog, ListSelect::GROUP_TARGET);
    opeu_control_message(game, Near(data, data.select_division), data.context_record2);
    if (!RunList(game, data.division_dialog)) return;
    auto const group{ data.list_row };
    auto&      division{ GroupOf(data, data.current_side, group) };
    auto const person{ opeu_find_group_record(game, data.current_side, group) };
    opeu_group_show(game, Near(data, division), static_cast<uint8_t>(group));
    if (division.flags & Bits(GroupFlag::AI_MARK)) {
      // The computer runs the division: edit its objective tasks, with the division current meanwhile.
      auto const saved{ data.current_group };
      data.current_group = group;
      SetListPredicate(data.order_target_list, ListSelect::ANY_ORDER_TARGET);
      for (;;) {
        opeu_control_message(game, Near(data, data.select_order), person);
        if (!RunList(game, data.order_target_list)) break;
        opeu_group_set_target(game, group);
      }
      data.current_group = saved;
    } else if (opeu_confirm_message(game, Near(data, data.order_whole_division), person)) {
      data.unit_order_active = 1;
      opeu_unit_order_menu(game, static_cast<uint8_t>(group));
      data.unit_order_active = 0;
    } else {
      data.organize_list.rows = static_cast<std::uint8_t>(opeu_order_units_fill(game, group) + 1);
      SetListPredicate(data.organize_list, ListSelect::SUPPLY_UNIT);
      for (;;) {
        opeu_control_message(game, Near(data, data.select_regiment), person);
        if (!RunList(game, data.organize_list)) break;
        opeu_unit_order_menu(game, static_cast<uint8_t>(group));
      }
    }
  }
}
