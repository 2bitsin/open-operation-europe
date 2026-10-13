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

using namespace opeu::game;

namespace {
// The link-table addresses of the list predicates in the Orders command's code segment.
enum Routine : std::uint16_t {
  GROUP_TARGET_ENABLED = 0x0006,
  MENU_ALWAYS          = 0x0040,
  SUPPLY_UNIT_ENABLED  = 0x021c,
};
// The Orders command's code segment in the routine link table.
constexpr std::uint16_t ORDERS_SEGMENT{ 0x1c23 };

auto RunList(Game& game, ListWindow& list) -> std::uint16_t {
  return opeu_list_dialog(game, svc_listdlg, Near(Data(game), list), static_cast<uint8_t>(LIST_LIMIT), static_cast<uint8_t>(LIST_FLAGS));
}
} // namespace

// Lists the eight regiments of the current side's division `group` in the regiment rows.
uint8_t opeu_order_units_fill(Game& game, uint8_t group) {
  auto& data{ Data(game) };
  for (std::uint8_t index{ 0 }; index < 8; ++index) data.row_units[index] = Near(data, data.units[(data.current_side * GROUPS_PER_SIDE + group) * 8 + index]);
  return 8;
}

// The Orders command; segment is its code segment.
void opeu_turn_action_1(Game& game, uint16_t segment) {
  auto& data{ Data(game) };
  opeu_title_panel_begin(game, static_cast<const DialogHost*>(port_dialogs_host()));
  data.unit_order_active = 0;
  for (;;) {
    SetListPredicate(data.division_dialog, GROUP_TARGET_ENABLED, segment);
    opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), Near(data, data.select_division), data.context_record2);
    if (!RunList(game, data.division_dialog)) return;
    auto const group{ data.list_row };
    auto&      division{ GroupOf(data, data.current_side, group) };
    auto const person{ opeu_find_group_record(game, data.current_side, group) };
    opeu_group_show(game, Near(data, division), static_cast<uint8_t>(group));
    if (division.flags & Bits(GroupFlag::AI_MARK)) {
      // The computer runs the division: edit its objective tasks, with the division current meanwhile.
      auto const saved{ data.current_group };
      data.current_group = group;
      SetListPredicate(data.order_target_list, MENU_ALWAYS, segment);
      for (;;) {
        opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), Near(data, data.select_order), person);
        if (!RunList(game, data.order_target_list)) break;
        opeu_group_set_target(game, group);
      }
      data.current_group = saved;
    } else if (opeu_confirm_message(game, static_cast<const DialogHost*>(port_dialogs_host()), Near(data, data.order_whole_division), person)) {
      data.unit_order_active = 1;
      opeu_unit_order_menu(game, static_cast<uint8_t>(group));
      data.unit_order_active = 0;
    } else {
      data.organize_list.rows = static_cast<std::uint8_t>(opeu_order_units_fill(game, group) + 1);
      SetListPredicate(data.organize_list, SUPPLY_UNIT_ENABLED, segment);
      for (;;) {
        opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), Near(data, data.select_regiment), person);
        if (!RunList(game, data.organize_list)) break;
        opeu_unit_order_menu(game, static_cast<uint8_t>(group));
      }
    }
  }
}

auto port_routine_opeu_turn_action_1(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_turn_action_1(game, ORDERS_SEGMENT);
  return 0;
}
