// The supply transfer between a regiment and its neighbour. The selection moves over the three resource rows (2
// down, 8 and 5 up); 1, 4 and 7 move 1, 10 and 100 to the left side within its capacity, 3, 6 and 9 to the right
// side within its capacity, which is unlimited while an objective is supplied. A player's side gives the keys; the
// computer's come from opeu_transfer_auto_key. A twin of the supply allocation (core/supstep).
#include "units/xferstep.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/exchange-panel.hpp"
#include "common/keys.hpp"
#include "common/memory.hpp"
#include "core/context.hpp"
#include "core/dialogs.hpp"
#include "core/limits.hpp"
#include "io/mouse.hpp"
#include "io/sound.hpp"
#include "io/soundcue.hpp"
#include "io/waitkey.hpp"
#include "map/support.hpp"
#include "types.hpp"
#include "units/transfer-service.hpp"
#include "units/xferpnl.hpp"

#include "common/game.hpp"
#include "port.hpp"
#include "services.hpp"

#include <cstddef>

using namespace opeu::game;

namespace {

// The computer's direction latch, cleared for a new transfer (units/xferpnl).
constexpr std::uint8_t UNMOVED{ '-' };
// The daily picture the player's transfer runs inside.
constexpr std::uint16_t TRANSFER_PICTURE{ 0x96 };
// What the computer's finished transfer passes to the pending presentation.
constexpr std::uint16_t FINISH_VALUE{ 3 };
// The context_side value that has the next daily context redrawn.
constexpr std::uint8_t NO_CONTEXT{ 0xff };

// The near pointer of a transfer_table word.
constexpr auto TableAt(unsigned side, unsigned row, unsigned column) -> std::uint16_t {
  return static_cast<std::uint16_t>(offsetof(DataSegment, transfer_table) + 2 * ((side * 3u + row) * 3u + column));
}

// Toggles the highlight of a row's name.
void Highlight(Game& game, std::uint8_t row) {
  auto const& data{ Data(game) };
  HighlightRow(game, { data.transfer_panel_x, data.transfer_panel_y }, row);
}
} // namespace

// Runs the transfer until Enter. A player's transfer asks for confirmation; the computer's finishes its presentation.
// The selected row's byte lives in the routine's reserve: the mouse input sets it through its near address.
void opeu_transfer_step(Game& game) {
  ServiceLocals const locals{ 2 };
  auto const          selection{ locals.At(0) };
  auto&               data{ Data(game) };
  auto&               selected{ Record<std::uint8_t>(data, selection) };
  std::uint8_t        highlighted{ NO_ROW };
  if (!data.transfer_interactive && SidePlayed(data, data.current_side)) opeu_sound_cue14(game);
  data.transfer_auto = UNMOVED;
  selected           = 0;
  opeu_transfer_draw_row(game, 1);
  opeu_transfer_draw_row(game, 0);
  opeu_transfer_draw_row(game, 2);
  opeu_input_reset(game);
  for (;;) {
    auto const left{ TableAt(0, selected, AMOUNT) };
    auto const right{ TableAt(1, selected, AMOUNT) };
    auto const left_capacity{ TableAt(0, selected, CAPACITY) };
    auto const right_capacity{ data.transfer_objective_ptr ? static_cast<std::uint16_t>(offsetof(DataSegment, transfer_unlimited)) : TableAt(1, selected, CAPACITY) };
    if (SidePlayed(data, data.current_side) && selected != highlighted) {
      if (highlighted != NO_ROW) Highlight(game, highlighted);
      Highlight(game, selected);
      highlighted = selected;
    }
    auto const key{ data.transfer_interactive ? opeu_transfer_input(game, selection) : opeu_transfer_auto_key(game, selected) };
    switch (key) {
    case '2':
      if (selected++ == RESOURCES - 1) selected = 0;
      break;
    case '5':
    case '8':
      if (selected-- == 0) selected = RESOURCES - 1;
      break;
    case '1':
    case '4':
    case '7': {
      auto const moved{ opeu_min16(opeu_min16(Word(game, left_capacity) - Word(game, left), KeyAmount(key)), Word(game, right)) };
      SetWord(game, left, Word(game, left) + moved);
      SetWord(game, right, Word(game, right) - moved);
      break;
    }
    case '3':
    case '6':
    case '9': {
      auto const moved{ opeu_min16(opeu_min16(Word(game, left), KeyAmount(key)), Word(game, right_capacity) - Word(game, right)) };
      SetWord(game, left, Word(game, left) - moved);
      SetWord(game, right, Word(game, right) + moved);
      break;
    }
    case KEY_ENTER:
      opeu_transfer_draw_row(game, selected);
      if (data.transfer_interactive) {
        data.transfer_interactive = opeu_confirm_ok(game, data.context_group_record);
        return;
      }
      opeu_sound_cue16(game);
      opeu_music_command7(game);
      opeu_pending_finish(game, static_cast<uint8_t>(FINISH_VALUE));
      opeu_daily_context(game, static_cast<uint8_t>(data.current_side));
      data.transfer_interactive = 1;
      return;
    }
    opeu_transfer_draw_row(game, selected);
  }
}

// Announces the transfer, runs the step (inside the daily picture for a player's side) and returns whether the
// transfer was confirmed.
uint16_t opeu_transfer_run(Game& game, uint16_t interactive) {
  auto& data{ Data(game) };
  if (interactive)
    opeu_unit_report(game, data.current_unit_ptr, offsetof(DataSegment, transfer_prompt));
  else if (SidePlayed(data, data.current_side) && opeu_unit_selection_eligible(game, data.current_unit_ptr))
    opeu_unit_report(game, data.current_unit_ptr, offsetof(DataSegment, transfer_begin));
  data.transfer_interactive = interactive;
  if (SidePlayed(data, data.current_side))
    opeu_daily_show(game, TRANSFER_PICTURE, static_cast<uint8_t>(data.current_side), Presenter::TRANSFER_STEP);
  else
    opeu_transfer_step(game);
  opeu_music_command7(game);
  data.context_side = NO_CONTEXT;
  opeu_daily_context(game, static_cast<uint8_t>(data.current_side));
  return data.transfer_interactive;
}
