// The interactive supply allocation. The player moves the selection over the three resource rows (2 down, 8 and 5
// up) and shifts amounts between the left and the right column of the supply table: 1, 4 and 7 move 1, 10 and 100
// to the left column within its capacity, 3, 6 and 9 to the right one within its capacity, which is unlimited
// (60000) while a city's stockpile is being set. Enter ends with the Operations chief's question.
#include "core/supstep.hpp"
#include "core/limits.hpp"
#include "core/suppanel.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "core/chief.hpp"
#include "core/suprow.hpp"
#include "glue.hpp"
#include "io/mouse.hpp"
#include "port.hpp"
#include "services.hpp"

#include <cstddef>

using namespace opeu::game;

namespace {

using enum SupplyService;

constexpr std::uint8_t  NO_ROW{ 0xff };
constexpr std::uint8_t  ROWS{ 3 };
constexpr std::uint8_t  ENTER{ 13 };
constexpr std::uint16_t ROW_HEIGHT{ 28 };
constexpr std::uint16_t HIGHLIGHT_COLOUR{ 0x107 };
constexpr std::uint8_t  CURRENT{ 0 };  // supply_table column: the amount
constexpr std::uint8_t  CAPACITY{ 1 }; // and the capacity

// The near pointer of a supply_table word.
constexpr auto TableAt(unsigned side, unsigned row, unsigned column) -> std::uint16_t {
  return static_cast<std::uint16_t>(offsetof(DataSegment, supply_table) + 2 * ((side * 3u + row) * 3u + column));
}

// Toggles the highlight of a row's name.
void Highlight(std::uint8_t* ds, HostEvent event, std::uint8_t row) {
  auto const& data{ Data(ds) };
  Call(ds, event, HIGHLIGHT, data.supply_panel_x + 0xd, row * ROW_HEIGHT + data.supply_panel_y + 0x23, 6, 0x1b, HIGHLIGHT_COLOUR);
}

// The amount a key moves: 1, 10 or 100.
auto KeyAmount(std::uint8_t key) -> std::uint16_t {
  switch (key) {
  case '1':
  case '3': return 1;
  case '4':
  case '6': return 10;
  default:  return 100;
  }
}
} // namespace

// The selected row's byte lives in the routine's reserve: the mouse input sets it through its near address.
uint16_t opeu_supply_step(uint8_t ds[65536], HostEvent event) {
  ServiceLocals const locals{ 2 };
  auto const          selection{ locals.At(0) };
  auto&               data{ Data(ds) };
  auto&               selected{ Record<std::uint8_t>(data, selection) };
  std::uint8_t        highlighted{ NO_ROW };
  selected = 0;
  Call(ds, event, ROW, 1, 0);
  Call(ds, event, ROW, 0, 0);
  Call(ds, event, ROW, 2, 0);
  Call(ds, event, CHIEF_TEXT, offsetof(DataSegment, supply_prompt));
  Call(ds, event, INPUT_RESET);
  for (;;) {
    std::uint8_t redraw{ 0 }; // the column the row redraw marks: 1 left, 2 right
    auto const   left{ TableAt(0, selected, CURRENT) };
    auto const   right{ TableAt(1, selected, CURRENT) };
    auto const   left_capacity{ TableAt(0, selected, CAPACITY) };
    auto const   right_capacity{ data.supply_objective ? static_cast<std::uint16_t>(offsetof(DataSegment, supply_unlimited)) : TableAt(1, selected, CAPACITY) };
    if (selected != highlighted) {
      if (highlighted != NO_ROW) Highlight(ds, event, highlighted);
      Highlight(ds, event, selected);
      highlighted = selected;
    }
    auto const key{ opeu_supply_input(ds, event, selection) };
    switch (key) {
    case '2':
      if (selected++ == ROWS - 1) selected = 0;
      break;
    case '5':
    case '8':
      if (selected-- == 0) selected = ROWS - 1;
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
    case ENTER:
      Call(ds, event, ROW, selected, redraw);
      Highlight(ds, event, highlighted);
      return Call(ds, event, CHIEF_WAIT, offsetof(DataSegment, supply_continue));
    }
    Call(ds, event, ROW, selected, redraw);
  }
}

uint16_t svc_supstep(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r{ 0 };
  switch (static_cast<SupplyService>(id)) {
  case MOUSE_SHOW:
  case INPUT_DELAY:
  case MOUSE_POSITION:
  case RIGHT_DOWN:
  case LEFT_DOWN:
  case MOUSE_HIDE:
  case KEY:            r = svc_suppanel(ds, id, args, count); break;
  case ROW:            opeu_supply_draw_row(ds, svc_suprow, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  case CHIEF_TEXT:     opeu_ops_chief_text(ds, svc_chief, Arg(args, count, 0)); break;
  case INPUT_RESET:    opeu_input_reset(ds, svc_mouse); break;
  case HIGHLIGHT:      opeu_ui_highlight(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4)); break;
  case CHIEF_WAIT:     r = opeu_ops_chief_wait(ds, svc_chief, Arg(args, count, 0)); break;
  default:             port_unhandled("supstep", id, count);
  }
  return r;
}
