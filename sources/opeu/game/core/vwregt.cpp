// The regiments views: the own side's divisions and their regiments, the sighted enemy regiments, and each listed
// regiment's battalions.
#include "core/vwregt.hpp"
#include "common/events.hpp"
#include "types.hpp"
#include "units/vwlist.hpp"

#include "common/data-segment.hpp"
#include "common/forces.hpp"
#include "common/view-service.hpp"
#include "core/vwarms.hpp"
#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <cstddef>

using namespace opeu::game;

namespace {

constexpr std::uint16_t REGIMENT_PROMPT{ offsetof(DataSegment, regiment_prompt) };
constexpr std::uint16_t REGIMENT_FORMAT{ offsetof(DataSegment, regiment_format) };
constexpr std::uint16_t DIVISION_PROMPT{ offsetof(DataSegment, view_division_prompt) };
constexpr std::uint16_t NONE_ENEMY{ offsetof(DataSegment, units_none_enemy) };
constexpr std::uint16_t LIST_WINDOW{ offsetof(DataSegment, list_window) };

// The locals whose addresses the services receive, as offsets in the routines' reserves: the regiment list's title,
// its regiment's name and its window state kept around the battalions; the division list's window state kept around
// the regiments.
constexpr std::uint16_t REGIMENT_TITLE_AT{ 0x00 };
constexpr std::uint16_t REGIMENT_NAME_AT{ 0x64 };
constexpr std::uint16_t REGIMENT_SAVED_WINDOW_AT{ 0x70 };
constexpr std::uint16_t REGIMENT_LOCALS{ 0x7a };
constexpr std::uint16_t DIVISION_SAVED_WINDOW_AT{ 0 };
constexpr std::uint16_t DIVISION_LOCALS{ 0xc };

} // namespace

// Picks a listed regiment, announces it and lists its battalions, the regiment list's window state kept aside.
void opeu_view_regiment_list(uint8_t ds[65536], HostEvent event) {
  ServiceLocals const locals{ REGIMENT_LOCALS };
  auto&               data{ Data(ds) };
  auto const          saved_window{ locals.At(REGIMENT_SAVED_WINDOW_AT) };
  auto const          name{ locals.At(REGIMENT_NAME_AT) };
  auto const          title{ locals.At(REGIMENT_TITLE_AT) };
  for (std::uint16_t height{ 7 }; Call(ds, event, ViewService::CONTROL_PANEL), Call(ds, event, ViewService::CONTROL_MESSAGE, REGIMENT_PROMPT, data.context_record3),
                                  Call(ds, event, ViewService::LIST_DIALOG, Near(data, data.organize_list), 1, height);
       height = 6) {
    auto const unit{ data.supply_units[data.list_row] };
    Call(ds, event, ViewService::UNIT_ANNOUNCE, unit, height);
    data.choice_dialog.rows = static_cast<uint8_t>(opeu_list_unit_elements(ds, event, unit) + 1);
    SetRowCheck(ds, event, data.choice_dialog, RowCheck::NONE_2);
    Call(ds, event, ViewService::CONTROL_PANEL);
    auto const unit_name{ Call(ds, event, ViewService::UNIT_NAME, unit, name) };
    Call(ds, event, ViewService::REQUEST_FORMAT, title, REGIMENT_FORMAT, unit_name);
    Call(ds, event, ViewService::CONTROL_MESSAGE, title, data.context_record3);
    Call(ds, event, ViewService::COPY_BYTES, LIST_WINDOW, saved_window, LIST_WINDOW_STATE);
    Call(ds, event, ViewService::LIST_DIALOG, Near(data, data.choice_dialog), LIST_LIMIT, LIST_FLAGS);
    Call(ds, event, ViewService::COPY_BYTES, saved_window, LIST_WINDOW, LIST_WINDOW_STATE);
  }
}

// Lists the eight regiments of one of the own side's divisions; returns 8.
uint8_t opeu_list_group_units(uint8_t ds[65536], HostEvent, uint8_t group) {
  auto&      data{ Data(ds) };
  auto const first{ (data.current_side * GROUPS_PER_SIDE + group) * UNITS_PER_GROUP };
  for (std::uint8_t n = 0; n < UNITS_PER_GROUP; n++) data.supply_units[n] = Near(data, data.units[first + n]);
  return UNITS_PER_GROUP;
}

// Picks one of the own side's divisions, shows it, then lists its regiments, the division list's window state kept
// aside.
void opeu_view_own_divisions(uint8_t ds[65536], HostEvent event) {
  ServiceLocals const locals{ DIVISION_LOCALS };
  auto&               data{ Data(ds) };
  auto const          saved_window{ locals.At(DIVISION_SAVED_WINDOW_AT) };
  for (std::uint16_t height{ 7 }; SetRowCheck(ds, event, data.division_dialog, RowCheck::DIVISION), Call(ds, event, ViewService::CONTROL_MESSAGE, DIVISION_PROMPT, data.context_record3),
                                  Call(ds, event, ViewService::LIST_DIALOG, Near(data, data.division_dialog), 1, height);
       height = 6) {
    auto const  division{ data.list_row };
    auto const& group{ GroupOf(data, data.current_side, division) };
    Call(ds, event, ViewService::GROUP_SHOW, Near(data, group), division);
    data.organize_list.rows = static_cast<uint8_t>(opeu_list_group_units(ds, event, division) + 1);
    SetRowCheck(ds, event, data.organize_list, RowCheck::REGIMENT);
    Call(ds, event, ViewService::COPY_BYTES, LIST_WINDOW, saved_window, LIST_WINDOW_STATE);
    opeu_view_regiment_list(ds, event);
    Call(ds, event, ViewService::COPY_BYTES, saved_window, LIST_WINDOW, LIST_WINDOW_STATE);
  }
}

// The sighted enemy regiments.
void opeu_view_enemy_units(uint8_t ds[65536], HostEvent event) {
  auto&        data{ Data(ds) };
  auto const   enemy{ static_cast<std::uint8_t>(Call(ds, event, ViewService::OPPOSITE_SIDE, data.current_side)) };
  std::uint8_t listed{ 0 };
  for (std::uint8_t n = 0; n < UNITS_PER_SIDE; n++) {
    auto const& unit{ data.units[enemy * UNITS_PER_SIDE + n] };
    if ((unit.flags & Bits(UnitFlag::READY)) && Call(ds, event, ViewService::UNIT_SPECIAL, Near(data, unit))) data.supply_units[listed++] = Near(data, unit);
  }
  if (listed == 0) {
    Call(ds, event, ViewService::CONTROL_MESSAGE, NONE_ENEMY, data.context_record3);
    return;
  }
  data.organize_list.rows = static_cast<uint8_t>(listed + 1);
  SetRowCheck(ds, event, data.organize_list, RowCheck::REGIMENT);
  opeu_view_regiment_list(ds, event);
}

uint16_t svc_vwregt(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  return svc_view(ds, "vwregt", id, args, count);
}

auto port_routine_opeu_view_enemy_units(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_view_enemy_units(ds, svc_vwregt);
  return 0;
}

auto port_routine_opeu_view_own_divisions(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_view_own_divisions(ds, svc_vwregt);
  return 0;
}
