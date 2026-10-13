// Staff advice menus. opeu_control_prepare clears the five advice buffers before a command turn. The advice menu
// re-shows a topic's buffered advice or runs the topic's action; command action 5 asks Operations or Intelligence
// until Intelligence reports a result of 1.
#include "core/advmenu.hpp"
#include "common/events.hpp"
#include "common/menu.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "core/dialogs.hpp"
#include "link/glue.hpp"
#include "link/link.hpp"
#include "port.hpp"
#include "services.hpp"

#include <cstddef>

using namespace opeu::game;

// Near pointers are DS offsets. Services leave the module through event:
// 1 control_message(text, person), 2 delay_dialog(text, last, person, menu), 3 far call(offset, segment) whose AX is
// returned.
namespace {

constexpr std::uint16_t ADVICE_PROMPT{ offsetof(DataSegment, advice_prompt) };
constexpr std::uint16_t ADVICE_MENU{ offsetof(DataSegment, advice_menu) };
constexpr std::uint16_t STAFF_PROMPT{ offsetof(DataSegment, staff_prompt) };
constexpr std::uint16_t STAFF_MENU{ offsetof(DataSegment, staff_menu) };
constexpr std::uint16_t INTELLIGENCE{ 2 }; // the staff menu's second item

// Calls an action through the far pointer the table holds.
auto CallAction(Game& game, FarPointer const& action) -> std::uint16_t {
  return static_cast<uint16_t>(port_call(game, static_cast<uint32_t>(static_cast<std::uint16_t>(action.segment)) * 16u + static_cast<std::uint16_t>(action.offset), 0, 0));
}
} // namespace

void opeu_control_prepare(Game& game) {
  for (auto& message : Data(game).advice_messages) message[0] = 0;
}

// The advice menu: topics 1-5 until cancelled.
void opeu_staff_advice_menu(Game& game) {
  auto& data{ Data(game) };
  for (;;) {
    std::uint16_t topic{ opeu_delay_dialog(game, static_cast<const DialogHost*>(port_dialogs_host()), ADVICE_PROMPT, 5, data.context_record2, ADVICE_MENU) };
    if (topic == MENU_CANCELLED) break;
    topic--;
    if (data.advice_messages[topic][0])
      opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), Near(data, data.advice_messages[topic]), data.context_record2);
    else
      CallAction(game, data.advice_actions[topic]);
  }
}

// Command action 5: Operations or Intelligence, until Intelligence reports a result of 1.
void opeu_turn_action_5(Game& game) {
  auto&         data{ Data(game) };
  std::uint16_t officer;
  std::uint16_t result;
  do {
    if ((officer = opeu_delay_dialog(game, static_cast<const DialogHost*>(port_dialogs_host()), STAFF_PROMPT, 2, data.context_record2, STAFF_MENU)) == MENU_CANCELLED) return;
    result = CallAction(game, data.staff_actions[officer]);
  } while (officer != INTELLIGENCE || result != 1);
}

auto port_routine_opeu_staff_advice_menu(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_staff_advice_menu(game);
  return 0;
}

auto port_routine_opeu_turn_action_5(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_turn_action_5(game);
  return 0;
}
