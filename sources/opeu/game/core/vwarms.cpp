// The information menu, the weapons views (the equipment types each side fields), the victory conditions and the view
// menu.
#include "core/vwarms.hpp"
#include "common/events.hpp"
#include "types.hpp"
#include "units/vwlist.hpp"

#include "common/data-segment.hpp"
#include "common/dispatch.hpp"
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
#include "core/vwcity.hpp"
#include "core/vwdiv.hpp"
#include "core/vwpeople.hpp"
#include "core/vwregt.hpp"
#include "io/listdlg.hpp"
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
// The units view's sides, by the enumerator info_actions holds for the choice.
constexpr auto INFO_ACTIONS = DispatchRows<InfoAction, auto (*)(Game&)->void>({
    { InfoAction::OWN,   opeu_view_own_divisions },
    { InfoAction::ENEMY, opeu_view_enemy_units   },
});

// The armaments view's sides, by the enumerator arms_actions holds for the choice.
constexpr auto ARMS_ACTIONS = DispatchRows<ArmsAction, auto (*)(Game&)->void>({
    { ArmsAction::OWN,   opeu_view_own_weapons   },
    { ArmsAction::ENEMY, opeu_view_enemy_weapons },
});

// The view menu's routines, by the enumerator view_main_actions holds for the choice.
constexpr auto VIEW_MAIN_ACTIONS = DispatchRows<ViewMainAction, auto (*)(Game&)->void>({
    { ViewMainAction::DIVISIONS, opeu_view_division_contact },
    { ViewMainAction::OFFICERS,  opeu_view_generals_menu    },
    { ViewMainAction::CITIES,    opeu_view_cities_menu      },
    { ViewMainAction::UNITS,     opeu_view_info_menu        },
    { ViewMainAction::ARMAMENTS, opeu_view_arms_menu        },
    { ViewMainAction::VICTORY,   opeu_view_victory          },
});

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
  for (std::uint16_t item; (item = opeu_delay_dialog(game, INFO_PROMPT, 2, data.context_record2, INFO_MENU)) != MENU_CANCELLED;) Dispatch(INFO_ACTIONS, data.info_actions.at(item - 1u), game);
}

// Picks a type from the weapon list, which ends at `end`, and shows its panel.
void opeu_view_weapon_list(Game& game, uint16_t end) {
  auto&      data{ Data(game) };
  auto const count{ static_cast<std::uint8_t>(static_cast<std::int16_t>(end - WEAPON_LIST) >> 1) };
  data.weapons_dialog.rows = static_cast<std::uint8_t>(count + 1);
  SetListPredicate(data.weapons_dialog, ListSelect::VIEW_ANY);
  for (std::uint16_t height{ 7 }; opeu_control_message(game, WEAPONS_PROMPT, data.context_record3), opeu_list_dialog(game, data.weapons_dialog, { 1, static_cast<std::uint8_t>(height) }); height = 6) {
    opeu_element_type_panel(game, data.weapon_list[data.list_row]);
    opeu_control_message(game, WEAPONS_HERE, data.context_record3);
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
  auto const enemy{ opeu_opposite_side(data.current_side) };
  ViewWeapons(game, data.side_masks[enemy]);
}

// The weapons menu (own, enemy) until cancelled.
void opeu_view_arms_menu(Game& game) {
  auto& data{ Data(game) };
  for (std::uint16_t item; (item = opeu_delay_dialog(game, ARMS_PROMPT, 2, data.context_record2, ARMS_MENU)) != MENU_CANCELLED;) Dispatch(ARMS_ACTIONS, data.arms_actions.at(item - 1u), game);
}

// The current side's victory conditions: side 0's are the scenario brief, side 1's begin the notes.
void opeu_view_victory(Game& game) {
  auto&      data{ Data(game) };
  auto const conditions{ data.current_side ? Near(data, data.scenario_notes) : Near(data, data.scenario_brief) };
  opeu_army_title(game, VICTORY_TITLE);
  opeu_cursor_print(game, std::array<std::uint16_t, 2>{ VICTORY_FORMAT, conditions });
  opeu_control_message(game, VICTORY_ORDERS, data.context_record3);
}

// The view menu: runs the picked action until action 1 has run (returns 1) or the menu is cancelled (returns 0).
uint16_t opeu_view_menu_loop(Game& game) {
  auto&         data{ Data(game) };
  std::uint16_t item;
  do {
    item = opeu_delay_dialog(game, VIEW_PROMPT, VIEW_MENU_ITEMS, data.context_record3, VIEW_MENU);
    if (item == MENU_CANCELLED) return 0;
    Dispatch(VIEW_MAIN_ACTIONS, data.view_main_actions.at(item - 1u), game);
  } while (item != 1);
  return 1;
}
