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

constexpr std::uint16_t KEYBOARD_PROMPT{ 117 }; // the delay prompt for keyboard input
constexpr std::uint16_t MOUSE_PROMPT{ 118 };    // the delay prompt for mouse input
constexpr std::uint16_t DIALOG_X{ 24 };

constexpr std::uint16_t PROMPT_BYTES{ 36 }; // the prompt the dialog service shows
} // namespace

// Copies the prompt into the routine's reserve, asks the context person for the hours and stores them as the current
// side's delay once confirmed.
void opeu_choose_turn_delay(Game& game) {
  ServiceLocals const locals{ PROMPT_BYTES };
  auto const          scratch{ locals.At(0) };
  auto&               data{ Data(game) };
  opeu_string_copy(game, scratch, opeu_message_lookup(game, data.input_mode == 0 ? KEYBOARD_PROMPT : MOUSE_PROMPT));
  auto const hours{ opeu_delay_dialog(game, static_cast<const DialogHost*>(port_dialogs_host()), scratch, DIALOG_X, data.context_record2, 0) };
  if (hours != MENU_CANCELLED && opeu_delay_confirm(game, static_cast<const DialogHost*>(port_dialogs_host()), data.context_record2, hours))
    data.side_delays[data.current_side] = static_cast<std::uint8_t>(hours);
}
