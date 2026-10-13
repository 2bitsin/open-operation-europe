// The Delay command: the side's commander picks how many hours the side's turn waits, and confirms it.
#include "core/delay.hpp"
#include "common/events.hpp"
#include "common/menu.hpp"
#include "core/strings.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "core/dialogs.hpp"
#include "core/msglook.hpp"
#include "link/glue.hpp"
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
void opeu_choose_turn_delay(Game& game, HostEvent event) {
  ServiceLocals const locals{ PROMPT_BYTES };
  auto const          scratch{ locals.At(0) };
  auto&               data{ Data(game) };
  opeu_string_copy(game, scratch, Call(game, event, TEXT, data.input_mode == 0 ? KEYBOARD_PROMPT : MOUSE_PROMPT));
  auto const hours{ Call(game, event, DIALOG, scratch, DIALOG_X, data.context_record2, 0) };
  if (hours != MENU_CANCELLED && Call(game, event, CONFIRM, hours, data.context_record2)) data.side_delays[data.current_side] = static_cast<std::uint8_t>(hours);
}

// The dialog takes the person before the hours.
uint16_t svc_delay(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r = 0;
  switch (id) {
  case TEXT:    r = opeu_message_lookup(game, svc_msglook, static_cast<std::int16_t>(Arg(args, count, 0))); break;
  case DIALOG:  r = opeu_delay_dialog(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3)); break;
  case CONFIRM: r = opeu_delay_confirm(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 1), static_cast<std::uint8_t>(Arg(args, count, 0))); break;
  default:      port_unhandled("delay", id, count);
  }
  return r;
}

auto port_routine_opeu_choose_turn_delay(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_choose_turn_delay(game, svc_delay);
  return 0;
}
