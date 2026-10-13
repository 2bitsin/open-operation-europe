// Contacting a division from the view menu: the division's commanders are introduced over its regiments and the
// daily view moves to the current regiment.
#include "core/vwdiv.hpp"
#include "common/events.hpp"
#include "core/armypanl.hpp"
#include "core/context.hpp"
#include "core/credits.hpp"
#include "core/dialogs.hpp"
#include "core/headings.hpp"
#include "core/people.hpp"
#include "core/selunit.hpp"
#include "core/sideutil.hpp"
#include "core/uiprim.hpp"
#include "map/mappos.hpp"
#include "map/mapunit.hpp"
#include "map/support.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/forces.hpp"
#include "common/game.hpp"
#include "common/view-service.hpp"
#include "core/vwarms.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <cstddef>

using namespace opeu::game;

namespace {

constexpr std::uint16_t CONTACT_PROMPT{ offsetof(DataSegment, contact_prompt) };
constexpr std::uint16_t CONTACT_MENU{ offsetof(DataSegment, contact_menu) };
constexpr std::uint16_t CONTACT_LOST{ offsetof(DataSegment, contact_lost) };
constexpr std::uint8_t  CANCELLED{ 0xff };     // the menu's answer when cancelled, as a byte
constexpr std::uint16_t ARMY_RECORD_KIND{ 1 }; // the side record that names the army's commander

} // namespace

// The contact menu, until cancelled: picks one of the current side's four divisions. A present division whose
// selected regiment may be selected is shown with its army and division commanders over its eight regiments, and
// the daily view moves to the current regiment; otherwise the contact fails.
void opeu_view_division_contact(Game& game) {
  auto&   data{ Data(game) };
  uint8_t answer;
  for (;;) {
    data.control_selection = 0;
    opeu_turn_active_begin(game, static_cast<DialogHost const*>(port_dialogs_host()));
    answer = static_cast<uint8_t>(opeu_delay_dialog(game, static_cast<DialogHost const*>(port_dialogs_host()), CONTACT_PROMPT, 4, static_cast<std::uint16_t>(data.context_record3), CONTACT_MENU));
    if (answer == CANCELLED) break;
    uint8_t const division = answer - 1;
    auto&         group{ GroupOf(data, data.current_side, division) };
    auto const    group_near{ Near(data, group) };
    if ((group.flags & Bits(GroupFlag::READY)) && opeu_unit_selection_eligible(game, static_cast<std::uint16_t>(opeu_find_group_selected_unit(game, static_cast<std::uint16_t>(group_near))))) {
      opeu_group_show(game, static_cast<std::uint16_t>(group_near), static_cast<std::uint8_t>(division));
      opeu_backdrop_fill_color(game, static_cast<UiHost const*>(port_uiprim_host()), static_cast<std::uint8_t>(data.current_side ? 1 : 2));
      auto const units{ Near(data, data.units[(division + data.current_side * GROUPS_PER_SIDE) * UNITS_PER_GROUP]) };
      auto const army{ opeu_find_side_record(game, static_cast<std::uint8_t>(data.current_side), static_cast<std::uint8_t>(ARMY_RECORD_KIND)) };
      auto const commander{ opeu_find_group_record(game, static_cast<std::uint8_t>(data.current_side), static_cast<std::uint8_t>(division)) };
      opeu_army_heading(game, army);
      opeu_division_heading(game, commander);
      opeu_division_regiments(game, static_cast<std::uint16_t>(units), commander);
      opeu_division_credits(game);
      opeu_screen_reset(game, static_cast<DialogHost const*>(port_dialogs_host()));
      opeu_daily_context(game, static_cast<std::uint8_t>(data.current_side));
      auto const here{ [&] -> Unit const& { return Record<Unit>(data, data.current_unit_ptr); } };
      opeu_control_position(game, static_cast<std::uint16_t>(here().x), static_cast<std::uint16_t>(here().y));
      opeu_arrival_location(game, static_cast<std::uint8_t>(here().x), static_cast<std::uint8_t>(here().y), static_cast<std::uint8_t>(data.current_side));
    } else
      opeu_control_message(game, static_cast<DialogHost const*>(port_dialogs_host()), CONTACT_LOST, static_cast<std::uint16_t>(data.context_record3));
  }
}

auto port_routine_opeu_view_division_contact(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_view_division_contact(game);
  return 0;
}
