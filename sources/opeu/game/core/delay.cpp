// The Delay command: the side's commander picks how many hours the side's turn waits, and confirms it.
#include "core/delay.hpp"
#include "common/events.hpp"
#include "common/menu.hpp"
#include "core/strings.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "core/dialogs.hpp"
#include "core/msglook.hpp"
#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {

// Services leave the module through event.
enum Service : std::uint16_t {
  TEXT    = 0, // (message id) -> near pointer to the text
  DIALOG  = 1, // (buffer, x, person, 0) -> the hours chosen, 0xffff when cancelled
  CONFIRM = 2, // (hours, person) -> nonzero when confirmed
};

constexpr std::uint16_t KEYBOARD_PROMPT{ 117 }; // the delay prompt for keyboard input
constexpr std::uint16_t MOUSE_PROMPT{ 118 };    // the delay prompt for mouse input
constexpr std::uint16_t DIALOG_X{ 24 };

constexpr std::uint16_t PROMPT_BYTES{ 36 }; // the prompt the dialog service shows
} // namespace

// Copies the prompt into the routine's reserve, asks the context person for the hours and stores them as the current
// side's delay once confirmed.
void opeu_choose_turn_delay(uint8_t ds[65536], HostEvent event) {
  ServiceLocals const locals{ PROMPT_BYTES };
  auto const          scratch{ locals.At(0) };
  auto&               data{ Data(ds) };
  opeu_string_copy(ds, scratch, Call(ds, event, TEXT, data.input_mode == 0 ? KEYBOARD_PROMPT : MOUSE_PROMPT));
  auto const hours{ Call(ds, event, DIALOG, scratch, DIALOG_X, data.context_record2, 0) };
  if (hours != MENU_CANCELLED && Call(ds, event, CONFIRM, hours, data.context_record2)) data.side_delays[data.current_side] = static_cast<std::uint8_t>(hours);
}

// The dialog takes the person before the hours.
uint16_t svc_delay(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r = 0;
  switch (id) {
  case TEXT:    r = opeu_message_lookup(ds, svc_msglook, static_cast<std::int16_t>(Arg(args, count, 0))); break;
  case DIALOG:  r = opeu_delay_dialog(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3)); break;
  case CONFIRM: r = opeu_delay_confirm(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 1), static_cast<std::uint8_t>(Arg(args, count, 0))); break;
  default:      port_unhandled("delay", id, count);
  }
  return r;
}

auto port_routine_opeu_choose_turn_delay(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_choose_turn_delay(ds, svc_delay);
  return 0;
}
