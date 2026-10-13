// The generals views: lists of the own and the enemy side's generals and the menu that picks one of them.
#include "core/vwpeople.hpp"
#include "common/events.hpp"
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
void opeu_view_people_list(Game& game, HostEvent event, uint8_t count) {
  auto& data{ Data(game) };
  data.people_dialog.rows = static_cast<uint8_t>(count + 1);
  SetRowCheck(game, event, data.people_dialog, RowCheck::NONE);
  Call(game, event, ViewService::CONTROL_PANEL);
  Call(game, event, ViewService::CONTROL_MESSAGE, PEOPLE_HERE, data.context_record3);
  Call(game, event, ViewService::LIST_DIALOG, Near(data, data.people_dialog), LIST_LIMIT, LIST_FLAGS);
}

// The own side's generals.
void opeu_view_own_generals(Game& game, HostEvent event) {
  auto&      data{ Data(game) };
  auto const count{ ListGenerals(data, data.current_side) };
  opeu_view_people_list(game, event, count);
}

// The enemy side's generals.
void opeu_view_enemy_generals(Game& game, HostEvent event) {
  auto&      data{ Data(game) };
  auto const enemy{ static_cast<std::uint8_t>(Call(game, event, ViewService::OPPOSITE_SIDE, data.current_side)) };
  auto const count{ ListGenerals(data, enemy) };
  if (count == 0) {
    Call(game, event, ViewService::CONTROL_MESSAGE, PEOPLE_NONE, data.context_record3);
    return;
  }
  opeu_view_people_list(game, event, count);
}

// The generals menu (own, enemy) until cancelled.
void opeu_view_generals_menu(Game& game, HostEvent event) {
  auto& data{ Data(game) };
  for (std::uint16_t item; (item = Call(game, event, ViewService::DELAY_DIALOG, PEOPLE_PROMPT, 2, data.context_record2, PEOPLE_MENU)) != MENU_CANCELLED;)
    RunAction(game, event, data.people_actions, item);
}

uint16_t svc_vwpeople(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  return svc_view(game, "vwpeople", id, args, count);
}

auto port_routine_opeu_view_enemy_generals(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_view_enemy_generals(game, svc_vwpeople);
  return 0;
}

auto port_routine_opeu_view_generals_menu(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_view_generals_menu(game, svc_vwpeople);
  return 0;
}

auto port_routine_opeu_view_own_generals(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_view_own_generals(game, svc_vwpeople);
  return 0;
}
