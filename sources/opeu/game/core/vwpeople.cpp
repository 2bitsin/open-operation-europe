// The generals views: lists of the own and the enemy side's generals and the menu that picks one of them.
#include "core/vwpeople.hpp"
#include "common/events.hpp"
#include "core/dialogs.hpp"
#include "core/sideutil.hpp"
#include "io/listdlg.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/view-service.hpp"
#include "core/vwarms.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <cstddef>

using namespace opeu::game;

namespace {

constexpr std::uint16_t PEOPLE_HERE{ offsetof(DataSegment, people_here) };
constexpr std::uint16_t PEOPLE_NONE{ offsetof(DataSegment, people_none) };
constexpr std::uint16_t PEOPLE_PROMPT{ offsetof(DataSegment, people_prompt) };
constexpr std::uint16_t PEOPLE_MENU{ offsetof(DataSegment, people_menu) };

// Lists the side's generals in service that are not deputies; returns their count.
auto ListGenerals(DataSegment& data, std::uint8_t side) -> std::uint8_t {
  std::uint8_t listed{ 0 };
  for (std::uint8_t n = 0; n < PEOPLE_PER_SIDE; n++) {
    auto const& person{ data.people[side * PEOPLE_PER_SIDE + n] };
    if (person.status != PERSON_VACANT && !(person.role & Bits(PersonRole::DEPUTY))) data.general_list[listed++] = Near(data, person);
  }
  return listed;
}

} // namespace

// Shows the listed generals.
void opeu_view_people_list(Game& game, uint8_t count) {
  auto& data{ Data(game) };
  data.people_dialog.rows = static_cast<uint8_t>(count + 1);
  SetRowCheck(game, data.people_dialog, RowCheck::NONE);
  opeu_title_panel_begin(game, static_cast<DialogHost const*>(port_dialogs_host()));
  opeu_control_message(game, static_cast<DialogHost const*>(port_dialogs_host()), PEOPLE_HERE, static_cast<std::uint16_t>(data.context_record3));
  opeu_list_dialog(game, static_cast<std::uint16_t>(Near(data, data.people_dialog)), static_cast<std::uint8_t>(LIST_LIMIT), static_cast<std::uint8_t>(LIST_FLAGS));
}

// The own side's generals.
void opeu_view_own_generals(Game& game) {
  auto&      data{ Data(game) };
  auto const count{ ListGenerals(data, data.current_side) };
  opeu_view_people_list(game, count);
}

// The enemy side's generals.
void opeu_view_enemy_generals(Game& game) {
  auto&      data{ Data(game) };
  auto const enemy{ static_cast<std::uint8_t>(opeu_opposite_side(static_cast<std::uint8_t>(data.current_side))) };
  auto const count{ ListGenerals(data, enemy) };
  if (count == 0) {
    opeu_control_message(game, static_cast<DialogHost const*>(port_dialogs_host()), PEOPLE_NONE, static_cast<std::uint16_t>(data.context_record3));
    return;
  }
  opeu_view_people_list(game, count);
}

// The generals menu (own, enemy) until cancelled.
void opeu_view_generals_menu(Game& game) {
  auto& data{ Data(game) };
  for (std::uint16_t item;
       (item = opeu_delay_dialog(game, static_cast<DialogHost const*>(port_dialogs_host()), PEOPLE_PROMPT, 2, static_cast<std::uint16_t>(data.context_record2), PEOPLE_MENU)) != MENU_CANCELLED;)
    RunAction(game, data.people_actions, item);
}

auto port_routine_opeu_view_enemy_generals(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_view_enemy_generals(game);
  return 0;
}

auto port_routine_opeu_view_generals_menu(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_view_generals_menu(game);
  return 0;
}

auto port_routine_opeu_view_own_generals(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_view_own_generals(game);
  return 0;
}
