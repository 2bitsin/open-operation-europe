// Staff advice menus. opeu_control_prepare clears the five advice buffers before a command turn. The advice menu
// re-shows a topic's buffered advice or runs the topic's action; command action 5 asks Operations or Intelligence
// until Intelligence reports a result of 1.
#include "types.hpp"
#include "core/advmenu.hpp"
#include "common/events.hpp"

#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "core/dialogs.hpp"
#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <cstddef>

using namespace opeu::game;

// Near pointers are DS offsets; sp is the DS offset of SP at entry. Services leave the module through event:
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
constexpr std::uint16_t CANCELLED{ 0xffff };
constexpr std::uint16_t INTELLIGENCE{ 2 }; // the staff menu's second item

// Calls an action through the far pointer the table holds; the pointer is first stored at the caller's frame slot
// (the far-call temporary).
auto CallAction(std::uint8_t* ds, HostEvent event, FarPointer const& action,
                std::uint16_t slot) -> std::uint16_t {
  auto&      data{ Data(ds) };
  auto const target{ action };
  Record<FarPointer>(data, slot) = target;
  return Call(ds, event, FAR_CALL, target.offset, target.segment);
}
} // namespace

void opeu_control_prepare(uint8_t ds[65536], HostEvent event, uint16_t sp) {
  (void)event;
  (void)sp;
  for (auto& message : Data(ds).advice_messages)
    message[0] = 0;
}

// The advice menu: topics 1-5 until cancelled.
void opeu_staff_advice_menu(uint8_t ds[65536], HostEvent event, uint16_t sp) {
  auto& data{ Data(ds) };
  for (;;) {
    std::uint16_t topic{ Call(ds, event, DELAY_DIALOG, ADVICE_PROMPT, 5,
                              data.context_record2, ADVICE_MENU) };
    if (topic == CANCELLED)
      break;
    topic--;
    if (data.advice_messages[topic][0])
      Call(ds, event, CONTROL_MESSAGE, Near(data, data.advice_messages[topic]),
           data.context_record2);
    else
      CallAction(ds, event, data.advice_actions[topic],
                 static_cast<std::uint16_t>(sp - 2 - 6));
  }
}

// Command action 5: Operations or Intelligence, until Intelligence reports a result of 1.
void opeu_turn_action_5(uint8_t ds[65536], HostEvent event, uint16_t sp) {
  auto&         data{ Data(ds) };
  std::uint16_t officer;
  std::uint16_t result;
  do {
    if ((officer = Call(ds, event, DELAY_DIALOG, STAFF_PROMPT, 2,
                        data.context_record2, STAFF_MENU)) == CANCELLED)
      return;
    result = CallAction(ds, event, data.staff_actions[officer],
                        static_cast<std::uint16_t>(sp - 2 - 8));
  } while (officer != INTELLIGENCE || result != 1);
}

// The services of the staff advice menus as direct calls.
uint16_t svc_advmenu(uint8_t* ds, uint16_t id, const uint16_t* args,
                     uint16_t count) {
  uint16_t r = 0;
  switch (id) {
    case CONTROL_MESSAGE:
      opeu_control_message(ds, (const DialogHost*)port_dialogs_host(),
                           Arg(args, count, 0), Arg(args, count, 1));
      break;
    case DELAY_DIALOG:
      r = opeu_delay_dialog(ds, (const DialogHost*)port_dialogs_host(),
                            Arg(args, count, 0), Arg(args, count, 1),
                            Arg(args, count, 2), Arg(args, count, 3));
      break;
    case FAR_CALL:
      r = (uint16_t)port_call(
          ds, (uint32_t)Arg(args, count, 1) * 16u + Arg(args, count, 0), 0, 0);
      break;
    default: port_unhandled("advmenu", id, count);
  }
  return r;
}

auto port_routine_opeu_staff_advice_menu(std::uint8_t* ds, std::uint16_t const*,
                                         unsigned) -> std::uint32_t {
  WithStack(PORT_FRAME, [&](std::uint16_t sp) {
    opeu_staff_advice_menu(ds, svc_advmenu, sp);
  });
  return 0;
}

auto port_routine_opeu_turn_action_5(std::uint8_t* ds, std::uint16_t const*,
                                     unsigned) -> std::uint32_t {
  WithStack(PORT_FRAME,
            [&](std::uint16_t sp) { opeu_turn_action_5(ds, svc_advmenu, sp); });
  return 0;
}
