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

enum Service : std::uint16_t {
  CONTROL_MESSAGE = 1,
  DELAY_DIALOG    = 2,
  FAR_CALL        = 3,
};

constexpr std::uint16_t ADVICE_PROMPT{ offsetof(DataSegment, advice_prompt) };
constexpr std::uint16_t ADVICE_MENU{ offsetof(DataSegment, advice_menu) };
constexpr std::uint16_t STAFF_PROMPT{ offsetof(DataSegment, staff_prompt) };
constexpr std::uint16_t STAFF_MENU{ offsetof(DataSegment, staff_menu) };
constexpr std::uint16_t INTELLIGENCE{ 2 }; // the staff menu's second item

// Calls an action through the far pointer the table holds.
auto CallAction(Game& game, HostEvent event, FarPointer const& action) -> std::uint16_t {
  return Call(game, event, FAR_CALL, action.offset, action.segment);
}
} // namespace

void opeu_control_prepare(Game& game) {
  for (auto& message : Data(game).advice_messages) message[0] = 0;
}

// The advice menu: topics 1-5 until cancelled.
void opeu_staff_advice_menu(Game& game, HostEvent event) {
  auto& data{ Data(game) };
  for (;;) {
    std::uint16_t topic{ opeu_delay_dialog(game, static_cast<const DialogHost*>(port_dialogs_host()), ADVICE_PROMPT, 5, data.context_record2, ADVICE_MENU) };
    if (topic == MENU_CANCELLED) break;
    topic--;
    if (data.advice_messages[topic][0])
      opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), Near(data, data.advice_messages[topic]), data.context_record2);
    else
      CallAction(game, event, data.advice_actions[topic]);
  }
}

// Command action 5: Operations or Intelligence, until Intelligence reports a result of 1.
void opeu_turn_action_5(Game& game, HostEvent event) {
  auto&         data{ Data(game) };
  std::uint16_t officer;
  std::uint16_t result;
  do {
    if ((officer = opeu_delay_dialog(game, static_cast<const DialogHost*>(port_dialogs_host()), STAFF_PROMPT, 2, data.context_record2, STAFF_MENU)) == MENU_CANCELLED) return;
    result = CallAction(game, event, data.staff_actions[officer]);
  } while (officer != INTELLIGENCE || result != 1);
}

// The services of the staff advice menus as direct calls.
uint16_t svc_advmenu(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r = 0;
  switch (id) {
  case CONTROL_MESSAGE: opeu_control_message(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case DELAY_DIALOG:    r = opeu_delay_dialog(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3)); break;
  case FAR_CALL:        r = (uint16_t)port_call(game, (uint32_t)Arg(args, count, 1) * 16u + Arg(args, count, 0), 0, 0); break;
  default:              port_unhandled("advmenu", id, count);
  }
  return r;
}

auto port_routine_opeu_staff_advice_menu(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_staff_advice_menu(game, svc_advmenu);
  return 0;
}

auto port_routine_opeu_turn_action_5(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_turn_action_5(game, svc_advmenu);
  return 0;
}
