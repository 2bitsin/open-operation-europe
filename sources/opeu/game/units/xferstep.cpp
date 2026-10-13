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
#include "core/limits.hpp"
#include "types.hpp"
#include "units/transfer-service.hpp"
#include "units/xferpnl.hpp"

#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <cstddef>

using namespace opeu::game;

namespace {

using enum TransferService;

// The computer's direction latch, cleared for a new transfer (units/xferpnl).
constexpr std::uint8_t UNMOVED{ '-' };
// The daily picture the player's transfer runs inside, and the step's own far address it runs as its presenter.
constexpr std::uint16_t TRANSFER_PICTURE{ 0x96 };
constexpr std::uint16_t STEP_OFFSET{ 0x05a8 };
constexpr std::uint16_t STEP_SEGMENT{ 0x31f4 };
// What the computer's finished transfer passes to the pending presentation.
constexpr std::uint16_t FINISH_VALUE{ 3 };
// The context_side value that has the next daily context redrawn.
constexpr std::uint8_t NO_CONTEXT{ 0xff };

// The near pointer of a transfer_table word.
constexpr auto TableAt(unsigned side, unsigned row, unsigned column) -> std::uint16_t {
  return static_cast<std::uint16_t>(offsetof(DataSegment, transfer_table) + 2 * ((side * 3u + row) * 3u + column));
}

// Toggles the highlight of a row's name.
void Highlight(std::uint8_t* ds, HostWordsEvent event, std::uint8_t row) {
  auto const& data{ Data(ds) };
  HighlightRow(ds, event, HIGHLIGHT, { data.transfer_panel_x, data.transfer_panel_y }, row);
}
} // namespace

// Runs the transfer until Enter. A player's transfer asks for confirmation; the computer's finishes its presentation.
// The selected row's byte lives in the routine's reserve: the mouse input sets it through its near address.
void opeu_transfer_step(uint8_t ds[65536], HostWordsEvent event) {
  ServiceLocals const locals{ 2 };
  auto const          selection{ locals.At(0) };
  auto&               data{ Data(ds) };
  auto&               selected{ Record<std::uint8_t>(data, selection) };
  std::uint8_t        highlighted{ NO_ROW };
  if (!data.transfer_interactive && SidePlayed(data, data.current_side)) Call(ds, event, SOUND_CUE_14);
  data.transfer_auto = UNMOVED;
  selected           = 0;
  opeu_transfer_draw_row(ds, event, 1, 0);
  opeu_transfer_draw_row(ds, event, 0, 0);
  opeu_transfer_draw_row(ds, event, 2, 0);
  Call(ds, event, INPUT_RESET);
  for (;;) {
    std::uint8_t redraw{ 0 }; // the column the row redraw marks: 1 left, 2 right
    auto const   left{ TableAt(0, selected, AMOUNT) };
    auto const   right{ TableAt(1, selected, AMOUNT) };
    auto const   left_capacity{ TableAt(0, selected, CAPACITY) };
    auto const   right_capacity{ data.transfer_objective_ptr ? static_cast<std::uint16_t>(offsetof(DataSegment, transfer_unlimited)) : TableAt(1, selected, CAPACITY) };
    if (SidePlayed(data, data.current_side) && selected != highlighted) {
      if (highlighted != NO_ROW) Highlight(ds, event, highlighted);
      Highlight(ds, event, selected);
      highlighted = selected;
    }
    auto const key{ data.transfer_interactive ? opeu_transfer_input(ds, event, selection) : opeu_transfer_auto_key(ds, selected) };
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
      auto const moved{ opeu_min16(opeu_min16(Word(ds, left_capacity) - Word(ds, left), KeyAmount(key)), Word(ds, right)) };
      SetWord(ds, left, Word(ds, left) + moved);
      SetWord(ds, right, Word(ds, right) - moved);
      redraw = 1;
      break;
    }
    case '3':
    case '6':
    case '9': {
      auto const moved{ opeu_min16(opeu_min16(Word(ds, left), KeyAmount(key)), Word(ds, right_capacity) - Word(ds, right)) };
      SetWord(ds, left, Word(ds, left) - moved);
      SetWord(ds, right, Word(ds, right) + moved);
      redraw = 2;
      break;
    }
    case KEY_ENTER:
      opeu_transfer_draw_row(ds, event, selected, redraw);
      if (data.transfer_interactive) {
        data.transfer_interactive = Call(ds, event, CONFIRM, data.context_group_record);
        return;
      }
      Call(ds, event, SOUND_CUE_16);
      Call(ds, event, MUSIC_COMMAND_7);
      Call(ds, event, PENDING_FINISH, FINISH_VALUE);
      Call(ds, event, DAILY_CONTEXT, data.current_side);
      data.transfer_interactive = 1;
      return;
    }
    opeu_transfer_draw_row(ds, event, selected, redraw);
  }
}

// Announces the transfer, runs the step (inside the daily picture for a player's side) and returns whether the
// transfer was confirmed.
uint16_t opeu_transfer_run(uint8_t ds[65536], HostWordsEvent event, uint16_t interactive) {
  auto& data{ Data(ds) };
  if (interactive)
    Call(ds, event, UNIT_REPORT, data.current_unit_ptr, offsetof(DataSegment, transfer_prompt));
  else if (SidePlayed(data, data.current_side) && Call(ds, event, UNIT_ELIGIBLE, data.current_unit_ptr))
    Call(ds, event, UNIT_REPORT, data.current_unit_ptr, offsetof(DataSegment, transfer_begin));
  data.transfer_interactive = interactive;
  if (SidePlayed(data, data.current_side))
    Call(ds, event, DAILY_SHOW, TRANSFER_PICTURE, data.current_side, STEP_OFFSET, STEP_SEGMENT);
  else
    opeu_transfer_step(ds, event);
  Call(ds, event, MUSIC_COMMAND_7);
  data.context_side = NO_CONTEXT;
  Call(ds, event, DAILY_CONTEXT, data.current_side);
  return data.transfer_interactive;
}

uint16_t svc_xferstep(uint8_t* ds, uint16_t id, uint16_t* args, uint16_t count) {
  return svc_transfer(ds, "xferstep", id, args, count);
}

auto port_routine_opeu_transfer_step(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_transfer_step(ds, svc_xferstep);
  return 0;
}
