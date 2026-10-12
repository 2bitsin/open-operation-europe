// Contacting a division from the view menu: the division's commanders are introduced over its regiments and the
// daily view moves to the current regiment.
#include "core/vwdiv.hpp"
#include "common/events.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/view-service.hpp"
#include "core/vwarms.hpp"
#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <cstddef>

using namespace opeu::game;

namespace {

constexpr std::uint16_t CONTACT_PROMPT{ offsetof(DataSegment, contact_prompt) };
constexpr std::uint16_t CONTACT_MENU{ offsetof(DataSegment, contact_menu) };
constexpr std::uint16_t CONTACT_LOST{ offsetof(DataSegment, contact_lost) };
constexpr std::uint8_t  CANCELLED{ 0xff }; // the menu's answer when cancelled, as a byte
constexpr std::uint16_t ARMY_RECORD_KIND{ 1 }; // the side record that names the army's commander

} // namespace

// The contact menu, until cancelled: picks one of the current side's four divisions. A present division whose
// selected regiment may be selected is shown with its army and division commanders over its eight regiments, and
// the daily view moves to the current regiment; otherwise the contact fails.
void opeu_view_division_contact(uint8_t ds[65536], HostEvent event, uint16_t) {
  auto&   data{ Data(ds) };
  uint8_t answer;
  for (;;) {
    data.control_selection = 0;
    Call(ds, event, ViewService::TURN_ACTIVE_BEGIN);
    answer = static_cast<uint8_t>(
        Call(ds, event, ViewService::DELAY_DIALOG, CONTACT_PROMPT, 4, data.context_record3, CONTACT_MENU));
    if (answer == CANCELLED) break;
    uint8_t const division = answer - 1;
    auto&         group{ data.groups[division + data.current_side * GROUPS_PER_SIDE] };
    auto const    group_near{ Near(data, group) };
    if ((group.flags & Bits(GroupFlag::READY))
        && Call(ds, event, ViewService::UNIT_ELIGIBLE, Call(ds, event, ViewService::GROUP_SELECTED_UNIT, group_near))) {
      Call(ds, event, ViewService::GROUP_SHOW, group_near, division);
      Call(ds, event, ViewService::BACKDROP_COLOR, data.current_side ? 1 : 2);
      auto const units{ Near(data, data.units[(division + data.current_side * GROUPS_PER_SIDE) * UNITS_PER_GROUP]) };
      auto const army{ Call(ds, event, ViewService::SIDE_RECORD, data.current_side, ARMY_RECORD_KIND) };
      auto const commander{ Call(ds, event, ViewService::GROUP_RECORD, data.current_side, division) };
      Call(ds, event, ViewService::ARMY_HEADING, army);
      Call(ds, event, ViewService::DIVISION_HEADING, commander);
      Call(ds, event, ViewService::DIVISION_UNITS, units, commander);
      Call(ds, event, ViewService::DIVISION_FINISH);
      Call(ds, event, ViewService::CONTROL_RESET);
      Call(ds, event, ViewService::DAILY_CONTEXT, data.current_side);
      auto const here{ [&] -> Unit const& { return Record<Unit>(data, data.current_unit_ptr); } };
      Call(ds, event, ViewService::CONTROL_POSITION, here().x, here().y);
      Call(ds, event, ViewService::ARRIVAL_LOCATION, here().x, here().y, data.current_side);
    } else
      Call(ds, event, ViewService::CONTROL_MESSAGE, CONTACT_LOST, data.context_record3);
  }
}

uint16_t svc_vwdiv(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  return svc_view(ds, "vwdiv", id, args, count);
}

auto port_routine_opeu_view_division_contact(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  WithStack(SERVICE_STACK, [&](std::uint16_t sp) { opeu_view_division_contact(ds, svc_vwdiv, sp); });
  return 0;
}
