// The information menu, the weapons views (the equipment types each side fields), the victory conditions, the view
// menu, and the services every information view calls (svc_view).
#include "core/vwarms.hpp"
#include "common/events.hpp"
#include "types.hpp"
#include "units/vwlist.hpp"

#include "common/data-segment.hpp"
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
#include "glue.hpp"
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

constexpr std::uint8_t  ELEMENT_KINDS{ 8 };          // the kinds the weapon list is collected by, in order
constexpr std::uint16_t VIEW_CODE_SEGMENT{ 0x1fe6 }; // the views' code segment, as the far call table numbers it
constexpr std::uint16_t VIEW_MENU_ITEMS{ 6 };

// Collects, kind by kind, the equipment types the nation bits allow into the weapon list and shows the list.
void ViewWeapons(std::uint8_t* ds, HostEvent event, std::uint8_t mask) {
  std::uint16_t end{ WEAPON_LIST };
  for (std::uint8_t kind = 0; kind < ELEMENT_KINDS; kind++) end = static_cast<std::uint16_t>(end + 2 * opeu_list_element_types(ds, event, end, mask, kind));
  opeu_view_weapon_list(ds, event, end);
}

// A service's argument that addresses a word in the data segment.
auto WordAt(std::uint8_t* ds, std::uint16_t at) -> std::uint16_t* {
  return &Record<std::uint16_t>(Data(ds), at);
}

} // namespace

// The information menu until cancelled.
void opeu_view_info_menu(uint8_t ds[65536], HostEvent event) {
  auto& data{ Data(ds) };
  for (std::uint16_t item; (item = Call(ds, event, ViewService::DELAY_DIALOG, INFO_PROMPT, 2, data.context_record2, INFO_MENU)) != MENU_CANCELLED;) RunAction(ds, event, data.info_actions, item);
}

// Picks a type from the weapon list, which ends at `end`, and shows its panel.
void opeu_view_weapon_list(uint8_t ds[65536], HostEvent event, uint16_t end) {
  auto&      data{ Data(ds) };
  auto const count{ static_cast<std::uint8_t>(static_cast<std::int16_t>(end - WEAPON_LIST) >> 1) };
  data.weapons_dialog.rows = static_cast<std::uint8_t>(count + 1);
  SetRowCheck(ds, event, data.weapons_dialog, RowCheck::ANY);
  for (std::uint16_t height{ 7 };
       Call(ds, event, ViewService::CONTROL_MESSAGE, WEAPONS_PROMPT, data.context_record3), Call(ds, event, ViewService::LIST_DIALOG, Near(data, data.weapons_dialog), 1, height); height = 6) {
    Call(ds, event, ViewService::TYPE_PANEL, data.weapon_list[data.list_row]);
    Call(ds, event, ViewService::CONTROL_MESSAGE, WEAPONS_HERE, data.context_record3);
  }
}

// The equipment types the own side fields.
void opeu_view_own_weapons(uint8_t ds[65536], HostEvent event) {
  auto& data{ Data(ds) };
  ViewWeapons(ds, event, data.side_masks[data.current_side]);
}

// The equipment types the enemy side fields.
void opeu_view_enemy_weapons(uint8_t ds[65536], HostEvent event) {
  auto&      data{ Data(ds) };
  auto const enemy{ static_cast<std::uint8_t>(Call(ds, event, ViewService::OPPOSITE_SIDE, data.current_side)) };
  ViewWeapons(ds, event, data.side_masks[enemy]);
}

// The weapons menu (own, enemy) until cancelled.
void opeu_view_arms_menu(uint8_t ds[65536], HostEvent event) {
  auto& data{ Data(ds) };
  for (std::uint16_t item; (item = Call(ds, event, ViewService::DELAY_DIALOG, ARMS_PROMPT, 2, data.context_record2, ARMS_MENU)) != MENU_CANCELLED;) RunAction(ds, event, data.arms_actions, item);
}

// The current side's victory conditions: side 0's are the scenario brief, side 1's begin the notes.
void opeu_view_victory(uint8_t ds[65536], HostEvent event) {
  auto&      data{ Data(ds) };
  auto const conditions{ data.current_side ? Near(data, data.scenario_notes) : Near(data, data.scenario_brief) };
  Call(ds, event, ViewService::ARMY_TITLE, VICTORY_TITLE);
  Call(ds, event, ViewService::ARMY_PRINT, VICTORY_FORMAT, conditions);
  Call(ds, event, ViewService::CONTROL_MESSAGE, VICTORY_ORDERS, data.context_record3);
}

// The view menu: runs the picked action until action 1 has run (returns 1) or the menu is cancelled (returns 0).
uint16_t opeu_view_menu_loop(uint8_t ds[65536], HostEvent event) {
  auto&         data{ Data(ds) };
  std::uint16_t item;
  do {
    item = Call(ds, event, ViewService::DELAY_DIALOG, VIEW_PROMPT, VIEW_MENU_ITEMS, data.context_record3, VIEW_MENU);
    if (item == MENU_CANCELLED) return 0;
    RunAction(ds, event, data.view_main_actions, item);
  } while (item != 1);
  return 1;
}

// The services of the information views as direct calls. Words are the callee's arguments, bytes zero-extended.
uint16_t svc_view(uint8_t* ds, const char* module, uint16_t id, const uint16_t* args, uint16_t count) {
  auto const arg{ [&](unsigned n) { return Arg(args, count, n); } };
  auto const byte{ [&](unsigned n) { return static_cast<std::uint8_t>(Arg(args, count, n)); } };
  auto const dialogs{ static_cast<DialogHost const*>(port_dialogs_host()) };
  uint16_t   r{ 0 };
  switch (static_cast<ViewService>(id)) {
  case ViewService::OBJECTIVE_SIDE:      r = opeu_objective_side(ds, arg(0)); break;
  case ViewService::OBJECTIVE_ELIGIBLE:  r = opeu_objective_eligible(ds, arg(0)); break;
  case ViewService::GROUP_SELECTED_UNIT: r = opeu_find_group_selected_unit(ds, arg(0)); break;
  case ViewService::UNIT_ELIGIBLE:       r = opeu_unit_selection_eligible(ds, arg(0)); break;
  case ViewService::UNIT_SIDE:           r = opeu_unit_side(arg(0)); break;
  case ViewService::ARMY_TITLE:          opeu_army_title(ds, dialogs, arg(0)); break;
  case ViewService::OBJECTIVE_PERCENTS:  opeu_objective_resource_percentages(ds, arg(0), WordAt(ds, arg(1)), WordAt(ds, arg(2)), WordAt(ds, arg(3))); break;
  case ViewService::ARMY_POSITION:       opeu_army_position(ds, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(arg(0)), static_cast<int16_t>(arg(1))); break;
  case ViewService::ARMY_PRINT:          opeu_cursor_print(ds, { args, count }); break;
  case ViewService::UNIT_NAME:           r = opeu_unit_name(ds, svc_unitname, arg(0), arg(1)); break;
  case ViewService::UNIT_CAPACITIES:     opeu_unit_capacities(ds, arg(0), WordAt(ds, arg(1)), WordAt(ds, arg(2)), WordAt(ds, arg(3))); break;
  case ViewService::RESOURCE_PERCENT:    r = opeu_resource_percent(arg(0), arg(2)); break;
  case ViewService::ORDER_DESTINATION:   r = opeu_unit_order_destination(ds, arg(0), &Record<std::uint8_t>(Data(ds), arg(1)), &Record<std::uint8_t>(Data(ds), arg(2))); break;
  case ViewService::LOCATION_NAME:       r = opeu_location_name(ds, svc_labels, byte(0), byte(1), arg(2)); break;
  case ViewService::GROUP_SHOW:          opeu_group_show(ds, svc_mapunit, arg(0), byte(1)); break;
  case ViewService::BACKDROP_COLOR:      opeu_backdrop_fill_color(ds, static_cast<UiHost const*>(port_uiprim_host()), byte(0)); break;
  case ViewService::SIDE_RECORD:         r = opeu_find_side_record(ds, byte(0), byte(1)); break;
  case ViewService::GROUP_RECORD:        r = opeu_find_group_record(ds, byte(0), byte(1)); break;
  case ViewService::ARMY_HEADING:        opeu_army_heading(ds, svc_headings, svc_headings_color, arg(0)); break;
  case ViewService::DIVISION_HEADING:    opeu_division_heading(ds, svc_headings, arg(0)); break;
  case ViewService::DIVISION_UNITS:      opeu_division_regiments(ds, svc_armypanl, arg(0), arg(1)); break;
  case ViewService::DIVISION_FINISH:     opeu_division_credits(ds, svc_credits); break;
  case ViewService::CONTROL_RESET:       opeu_screen_reset(ds, dialogs); break;
  case ViewService::DAILY_CONTEXT:       opeu_daily_context(ds, svc_context, byte(0)); break;
  case ViewService::CONTROL_POSITION:    opeu_control_position(ds, arg(0), arg(1)); break;
  case ViewService::ARRIVAL_LOCATION:    opeu_arrival_location(ds, svc_mappos, byte(0), byte(1), byte(2)); break;
  case ViewService::CONTROL_MESSAGE:     opeu_control_message(ds, dialogs, arg(0), arg(1)); break;
  case ViewService::TURN_ACTIVE_BEGIN:   opeu_turn_active_begin(ds, dialogs); break;
  case ViewService::DELAY_DIALOG:        r = opeu_delay_dialog(ds, dialogs, arg(0), arg(1), arg(2), arg(3)); break;
  case ViewService::CONTROL_PANEL:       opeu_title_panel_begin(ds, dialogs); break;
  case ViewService::LIST_DIALOG:         r = opeu_list_dialog(ds, svc_listdlg, arg(0), byte(1), byte(2)); break;
  case ViewService::OPPOSITE_SIDE:       r = opeu_opposite_side(byte(0)); break;
  case ViewService::UNIT_FLASH:          opeu_unit_flash(ds, svc_mapunit, arg(0), byte(1)); break;
  case ViewService::REQUEST_FORMAT:      opeu_request_format(ds, { args, count }); break;
  case ViewService::COPY_BYTES:          opeu_copy_bytes(ds, arg(0), arg(1), arg(2)); break;
  case ViewService::UNIT_ANNOUNCE:       opeu_unit_announce(ds, svc_mapunit, arg(0)); break;
  case ViewService::UNIT_SPECIAL:        r = opeu_unit_special(ds, arg(0)); break;
  case ViewService::TYPE_PANEL:          opeu_element_type_panel(ds, dialogs, arg(0)); break;
  case ViewService::FAR_CALL:            r = static_cast<uint16_t>(port_call(ds, static_cast<uint32_t>(arg(1)) * 16u + arg(0), nullptr, 0)); break;
  case ViewService::CODE_SEGMENT:        r = VIEW_CODE_SEGMENT; break;
  default:                               port_unhandled(module, id, count);
  }
  return r;
}

uint16_t svc_vwarms(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  return svc_view(ds, "vwarms", id, args, count);
}

auto port_routine_opeu_view_arms_menu(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_view_arms_menu(ds, svc_vwarms);
  return 0;
}

auto port_routine_opeu_view_enemy_weapons(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_view_enemy_weapons(ds, svc_vwarms);
  return 0;
}

auto port_routine_opeu_view_info_menu(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_view_info_menu(ds, svc_vwarms);
  return 0;
}

auto port_routine_opeu_view_menu_loop(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  return opeu_view_menu_loop(ds, svc_vwarms);
}

auto port_routine_opeu_view_own_weapons(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_view_own_weapons(ds, svc_vwarms);
  return 0;
}

auto port_routine_opeu_view_victory(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_view_victory(ds, svc_vwarms);
  return 0;
}
