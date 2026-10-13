// The information menu, the weapons views (the equipment types each side fields), the victory conditions and the view
// menu.
#include "core/vwarms.hpp"
#include "common/events.hpp"
#include "types.hpp"
#include "units/vwlist.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/view-service.hpp"
#include "core/armypanl.hpp"
#include "core/context.hpp"
#include "core/credits.hpp"
#include "core/dialogs.hpp"
#include "core/headings.hpp"
#include "core/memory.hpp"
#include "core/people.hpp"
#include "core/recindex.hpp"
#include "core/selunit.hpp"
#include "core/sideutil.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "io/listdlg.hpp"
#include "link/glue.hpp"
#include "link/link.hpp"
#include "map/mappos.hpp"
#include "map/mapunit.hpp"
#include "map/objeleg.hpp"
#include "map/query.hpp"
#include "map/support.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/capacity.hpp"
#include "units/labels.hpp"
#include "units/objpct.hpp"
#include "units/orders.hpp"
#include "units/ratio.hpp"
#include "units/stats.hpp"
#include "units/unitname.hpp"

#include <array>
#include <cstddef>

using namespace opeu::game;

namespace {

constexpr std::uint16_t INFO_PROMPT{ offsetof(DataSegment, info_prompt) };
constexpr std::uint16_t INFO_MENU{ offsetof(DataSegment, info_menu) };
constexpr std::uint16_t ARMS_PROMPT{ offsetof(DataSegment, arms_prompt) };
constexpr std::uint16_t ARMS_MENU{ offsetof(DataSegment, arms_menu) };
constexpr std::uint16_t VIEW_PROMPT{ offsetof(DataSegment, view_main_prompt) };
constexpr std::uint16_t VIEW_MENU{ offsetof(DataSegment, view_main_menu) };
constexpr std::uint16_t WEAPONS_PROMPT{ offsetof(DataSegment, weapons_prompt) };
constexpr std::uint16_t WEAPONS_HERE{ offsetof(DataSegment, weapons_here) };
constexpr std::uint16_t WEAPON_LIST{ offsetof(DataSegment, weapon_list) };
constexpr std::uint16_t VICTORY_TITLE{ offsetof(DataSegment, victory_title) };
constexpr std::uint16_t VICTORY_FORMAT{ offsetof(DataSegment, victory_format) };
constexpr std::uint16_t VICTORY_ORDERS{ offsetof(DataSegment, victory_orders) };

constexpr std::uint8_t  ELEMENT_KINDS{ 8 }; // the kinds the weapon list is collected by, in order
constexpr std::uint16_t VIEW_MENU_ITEMS{ 6 };

// Collects, kind by kind, the equipment types the nation bits allow into the weapon list and shows the list.
void ViewWeapons(Game& game, std::uint8_t mask) {
  std::uint16_t end{ WEAPON_LIST };
  for (std::uint8_t kind = 0; kind < ELEMENT_KINDS; kind++) end = static_cast<std::uint16_t>(end + 2 * opeu_list_element_types(game, end, mask, kind));
  opeu_view_weapon_list(game, end);
}

} // namespace

// The information menu until cancelled.
void opeu_view_info_menu(Game& game) {
  auto& data{ Data(game) };
  for (std::uint16_t item;
       (item = opeu_delay_dialog(game, static_cast<DialogHost const*>(port_dialogs_host()), INFO_PROMPT, 2, static_cast<std::uint16_t>(data.context_record2), INFO_MENU)) != MENU_CANCELLED;)
    RunAction(game, data.info_actions, item);
}

// Picks a type from the weapon list, which ends at `end`, and shows its panel.
void opeu_view_weapon_list(Game& game, uint16_t end) {
  auto&      data{ Data(game) };
  auto const count{ static_cast<std::uint8_t>(static_cast<std::int16_t>(end - WEAPON_LIST) >> 1) };
  data.weapons_dialog.rows = static_cast<std::uint8_t>(count + 1);
  SetRowCheck(game, data.weapons_dialog, RowCheck::ANY);
  for (std::uint16_t height{ 7 }; opeu_control_message(game, static_cast<DialogHost const*>(port_dialogs_host()), WEAPONS_PROMPT, static_cast<std::uint16_t>(data.context_record3)),
                                  opeu_list_dialog(game, static_cast<std::uint16_t>(Near(data, data.weapons_dialog)), static_cast<std::uint8_t>(1), static_cast<std::uint8_t>(height));
       height = 6) {
    opeu_element_type_panel(game, static_cast<DialogHost const*>(port_dialogs_host()), static_cast<std::uint16_t>(data.weapon_list[data.list_row]));
    opeu_control_message(game, static_cast<DialogHost const*>(port_dialogs_host()), WEAPONS_HERE, static_cast<std::uint16_t>(data.context_record3));
  }
}

// The equipment types the own side fields.
void opeu_view_own_weapons(Game& game) {
  auto& data{ Data(game) };
  ViewWeapons(game, data.side_masks[data.current_side]);
}

// The equipment types the enemy side fields.
void opeu_view_enemy_weapons(Game& game) {
  auto&      data{ Data(game) };
  auto const enemy{ static_cast<std::uint8_t>(opeu_opposite_side(static_cast<std::uint8_t>(data.current_side))) };
  ViewWeapons(game, data.side_masks[enemy]);
}

// The weapons menu (own, enemy) until cancelled.
void opeu_view_arms_menu(Game& game) {
  auto& data{ Data(game) };
  for (std::uint16_t item;
       (item = opeu_delay_dialog(game, static_cast<DialogHost const*>(port_dialogs_host()), ARMS_PROMPT, 2, static_cast<std::uint16_t>(data.context_record2), ARMS_MENU)) != MENU_CANCELLED;)
    RunAction(game, data.arms_actions, item);
}

// The current side's victory conditions: side 0's are the scenario brief, side 1's begin the notes.
void opeu_view_victory(Game& game) {
  auto&      data{ Data(game) };
  auto const conditions{ data.current_side ? Near(data, data.scenario_notes) : Near(data, data.scenario_brief) };
  opeu_army_title(game, static_cast<DialogHost const*>(port_dialogs_host()), VICTORY_TITLE);
  opeu_cursor_print(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(VICTORY_FORMAT), static_cast<std::uint16_t>(conditions) });
  opeu_control_message(game, static_cast<DialogHost const*>(port_dialogs_host()), VICTORY_ORDERS, static_cast<std::uint16_t>(data.context_record3));
}

// The view menu: runs the picked action until action 1 has run (returns 1) or the menu is cancelled (returns 0).
uint16_t opeu_view_menu_loop(Game& game) {
  auto&         data{ Data(game) };
  std::uint16_t item;
  do {
    item = opeu_delay_dialog(game, static_cast<DialogHost const*>(port_dialogs_host()), VIEW_PROMPT, VIEW_MENU_ITEMS, static_cast<std::uint16_t>(data.context_record3), VIEW_MENU);
    if (item == MENU_CANCELLED) return 0;
    RunAction(game, data.view_main_actions, item);
  } while (item != 1);
  return 1;
}

auto port_routine_opeu_view_arms_menu(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_view_arms_menu(game);
  return 0;
}

auto port_routine_opeu_view_enemy_weapons(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_view_enemy_weapons(game);
  return 0;
}

auto port_routine_opeu_view_info_menu(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_view_info_menu(game);
  return 0;
}

auto port_routine_opeu_view_menu_loop(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  return opeu_view_menu_loop(game);
}

auto port_routine_opeu_view_own_weapons(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_view_own_weapons(game);
  return 0;
}

auto port_routine_opeu_view_victory(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_view_victory(game);
  return 0;
}
