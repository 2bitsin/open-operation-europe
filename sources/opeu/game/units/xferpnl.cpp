// The supply-transfer panel of the supply orders: the keypad drawn on it, mouse and keyboard input, the transfer
// table (per side and resource: the amount, the capacity and the computer's target), the names in its title, the
// rows with their fill percentages and the computer's keys. A twin of the supply-allocation panel (core/suppanel).
#include "types.hpp"
#include "units/xferpnl.hpp"
#include "core/strings.hpp"
#include "units/ratio.hpp"
#include "types/events.hpp"
#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "units/transfer-service.hpp"

#include "port.hpp"
#include "glue.hpp"
#include "services.hpp"
#include "core/context.hpp"
#include "core/dialogs.hpp"
#include "core/emsmap.hpp"
#include "core/recindex.hpp"
#include "core/textout.hpp"
#include "io/keyboard.hpp"
#include "io/mouse.hpp"
#include "io/sound.hpp"
#include "io/soundcue.hpp"
#include "io/waitkey.hpp"
#include "map/movetgt.hpp"
#include "map/support.hpp"
#include "units/aggpct.hpp"
#include "units/capacity.hpp"
#include "units/orders.hpp"
#include "units/resagg.hpp"
#include "units/stats.hpp"
#include "units/unitname.hpp"
#include "units/value.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <utility>

using namespace opeu::game;

namespace {

using enum TransferService;

// The keypad: three key columns and three key rows, in pixels from the panel's left edge and top.
constexpr std::array<std::uint16_t, 4> KEY_COLUMNS{ 0x48, 0x68, 0x98, 0xb8 };
constexpr std::array<std::uint16_t, 4> KEY_ROWS{ 0x23, 0x3f, 0x5b, 0x77 };
constexpr std::uint8_t                 KEYS[3][3]{ { '7', '4', '1' },
                                                   { '8', '5', '2' },
                                                   { '9', '6', '3' } };
constexpr std::uint8_t                 NO_KEY{ 0xff };
constexpr std::uint8_t                 ENTER{ 13 };

// The panel's background, saved in the number dialog's image segment.
constexpr std::uint16_t PANEL_WIDTH{ 0x20 }; // in 8-pixel columns
constexpr std::uint16_t PANEL_HEIGHT{ 0x78 };
constexpr std::uint16_t PANEL_IMAGE_AT{ 0x1f00 };

constexpr std::int16_t  NAME_LIMIT{ 10 }; // characters of a name in the title
constexpr std::uint16_t ROW_HEIGHT{ 28 };

constexpr std::uint16_t GLYPH_SIZE{
  6
}; // digit glyphs are 6 x 6 pixels, 6 bytes each
constexpr std::uint16_t BLANK_GLYPH{ 10 };
constexpr std::uint16_t GLYPH_COLOUR{ 7 };
constexpr std::uint16_t BOX_WIDTH{
  3
}; // the percentage's background, in 8-pixel columns
constexpr std::uint16_t BOX_HEIGHT{ 6 };

// The computer's direction latch: which way it last moved supplies, so that it never moves them back.
constexpr std::uint8_t TOWARD_LEFT{ '<' };
constexpr std::uint8_t TOWARD_RIGHT{ '>' };
constexpr std::uint8_t UNMOVED{ '-' };

auto Played(DataSegment const& data) -> bool {
  return data.side_status[data.current_side] == SIDE_PLAYED;
}

auto MousePosition(std::uint8_t* ds, HostWordsEvent event)
    -> std::array<std::uint16_t, 2> {
  std::array<std::uint16_t, 2> position{};
  event(ds, std::to_underlying(MOUSE_POSITION), position.data(), 0);
  return position;
}
} // namespace

// The key under a point of the keypad, NO_KEY off it.
uint8_t opeu_transfer_keypad_hit(uint8_t ds[65536], uint16_t x, uint16_t y) {
  auto const&  data{ Data(ds) };
  auto const   left{ static_cast<std::uint16_t>(data.transfer_panel_x << 3) };
  auto const   top{ data.transfer_panel_y };
  std::uint8_t key{ NO_KEY };
  for (auto column = 0u; column < 3; ++column) {
    if (static_cast<std::uint16_t>(left + KEY_COLUMNS[column]) > x ||
        static_cast<std::uint16_t>(left + KEY_COLUMNS[column + 1]) <= x)
      continue;
    for (auto row = 0u; row < 3; ++row)
      if (static_cast<std::uint16_t>(top + KEY_ROWS[row]) <= y &&
          static_cast<std::uint16_t>(top + KEY_ROWS[row + 1]) > y)
        key = KEYS[column][row];
  }
  return key;
}

// With the mouse: the right button is Enter and a left click on the keys 8, 5 or 2 selects row 0, 1 or 2 (stored at
// the near pointer row, NO_KEY returned). With the keyboard: the key.
uint8_t opeu_transfer_input(uint8_t ds[65536], HostWordsEvent event,
                            uint16_t row) {
  auto& data{ Data(ds) };
  if (!data.input_mode)
    return static_cast<std::uint8_t>(Call(ds, event, KEY));
  Call(ds, event, MOUSE_SHOW);
  Call(ds, event, INPUT_DELAY, 3);
  std::uint8_t key;
  for (;;) {
    auto const [x, y]{ MousePosition(ds, event) };
    if (Call(ds, event, RIGHT_DOWN)) {
      key = ENTER;
      break;
    }
    if (Call(ds, event, LEFT_DOWN)) {
      key = opeu_transfer_keypad_hit(ds, x, y);
      for (std::uint8_t selected = 0; selected < RESOURCES; ++selected)
        if (key == KEYS[1][selected]) {
          Record<std::uint8_t>(data, row) = selected;
          key                             = NO_KEY;
        }
      break;
    }
  }
  Call(ds, event, MOUSE_HIDE);
  return key;
}

void opeu_transfer_set(uint8_t ds[65536], uint8_t side, uint8_t row,
                       uint8_t column, uint16_t value) {
  Data(ds).transfer_table[side][row][column] = value;
}

uint16_t opeu_transfer_get(const uint8_t ds[65536], uint8_t side, uint8_t row,
                           uint8_t column) {
  return Data(ds).transfer_table[side][row][column];
}

// For a player's side: titles the panel, restores its background and centres the two names, at most ten characters
// each.
void opeu_transfer_draw(uint8_t ds[65536], HostWordsEvent event, uint16_t first,
                        uint16_t second) {
  auto const& data{ Data(ds) };
  if (!Played(data))
    return;
  Call(ds, event, TITLE, offsetof(DataSegment, transfer_title));
  Call(ds, event, MAP_DIALOG);
  Call(ds, event, RESTORE, data.transfer_panel_x, data.transfer_panel_y,
       PANEL_WIDTH, PANEL_HEIGHT, 0, data.numdlg_image_segment, PANEL_IMAGE_AT);
  auto const indent{ [&](std::uint16_t name) {
    auto const length{ std::min(
        static_cast<std::int16_t>(opeu_string_length(ds, name)), NAME_LIMIT) };
    return static_cast<std::int16_t>(NAME_LIMIT - length) / 2;
  } };
  Call(ds, event, POSITION, indent(first) + data.transfer_panel_x + 2,
       data.transfer_panel_y + 14);
  Call(ds, event, PRINT, offsetof(DataSegment, transfer_name_format), first);
  Call(ds, event, POSITION, indent(second) + data.transfer_panel_x + 20,
       data.transfer_panel_y + 14);
  Call(ds, event, PRINT, offsetof(DataSegment, transfer_name_text), second);
}

// Draws value right-aligned leftwards from x as digit glyphs over blanks; zero draws one digit.
void opeu_transfer_digits(uint8_t ds[65536], HostWordsEvent event, uint16_t x,
                          uint16_t y, uint16_t value) {
  auto const glyphs{ Near(Data(ds), Data(ds).digit_glyphs) };
  for (;; x -= GLYPH_SIZE) {
    Call(ds, event, GLYPH, x, y, GLYPH_SIZE, GLYPH_SIZE, 0,
         glyphs + GLYPH_SIZE * BLANK_GLYPH);
    Call(ds, event, GLYPH, x, y, GLYPH_SIZE, GLYPH_SIZE, GLYPH_COLOUR,
         glyphs + GLYPH_SIZE * (value % 10));
    if (!(value /= 10))
      break;
  }
}

// For a player's side, redraws a resource row: both amounts and both fill percentages. column (the column just
// changed) is not used.
void opeu_transfer_draw_row(uint8_t ds[65536], HostWordsEvent event,
                            uint8_t row, uint8_t column) {
  (void)column;
  auto const& data{ Data(ds) };
  if (!Played(data))
    return;
  auto const& left{ data.transfer_table[0][row] };
  auto const& right{ data.transfer_table[1][row] };
  auto const  left_amount{ left[AMOUNT] };
  auto const  right_amount{ right[AMOUNT] };
  auto const left_percent{ opeu_resource_percent(left_amount, left[CAPACITY]) };
  auto const right_percent{ opeu_resource_percent(right_amount,
                                                  right[CAPACITY]) };
  auto const x{ data.transfer_panel_x };
  auto const text_y{ row * ROW_HEIGHT + data.transfer_panel_y + 0x24 };
  auto const box_y{ static_cast<std::uint16_t>(row * ROW_HEIGHT +
                                               data.transfer_panel_y + 0x37) };
  Call(ds, event, POSITION, x + 2, text_y);
  Call(ds, event, PRINT, offsetof(DataSegment, transfer_amount_format),
       left_amount);
  Call(ds, event, POSITION, x + 0x19, text_y);
  Call(ds, event, PRINT, offsetof(DataSegment, transfer_amount_text),
       right_amount);
  Call(ds, event, BOX, x + 2, box_y, BOX_WIDTH, BOX_HEIGHT, 0);
  opeu_transfer_digits(ds, event, static_cast<std::uint16_t>((x + 4) << 3),
                       box_y, left_percent);
  Call(ds, event, BOX, x + 0x19, box_y, BOX_WIDTH, BOX_HEIGHT, 0);
  opeu_transfer_digits(ds, event, static_cast<std::uint16_t>((x + 0x1b) << 3),
                       box_y, right_percent);
  Call(ds, event, PRINT, offsetof(DataSegment, transfer_colour));
}

// The computer's key for a resource row. Supplies move to the left side ('7', '4', '1') while its target exceeds its
// amount and the right side holds more than its target, or to the right side ('9', '6', '3') in the opposite case;
// the step compares the unsigned difference with 100 and 10 (so a move to the left is always '7'). The direction
// latch keeps the computer from moving supplies back. Otherwise '2' selects the next row, and Enter ends after the
// last one.
uint8_t opeu_transfer_auto_key(uint8_t ds[65536], uint8_t row) {
  auto&       data{ Data(ds) };
  auto&       latch{ data.transfer_auto };
  auto const& left{ data.transfer_table[0][row] };
  auto const& right{ data.transfer_table[1][row] };
  auto const  left_amount{ left[AMOUNT] };
  auto const  right_amount{ right[AMOUNT] };
  auto const  left_target{ left[TARGET] };
  auto const  right_target{ right[TARGET] };
  if (latch != TOWARD_RIGHT && left_target > left_amount &&
      right_target < right_amount) {
    latch = TOWARD_LEFT;
    auto const step{ static_cast<std::uint16_t>(right_target - right_amount) };
    return step >= 100 ? '7' : step >= 10 ? '4' : '1';
  }
  if (latch != TOWARD_LEFT && left_target < left_amount &&
      right_target > right_amount) {
    latch = TOWARD_RIGHT;
    auto const step{ static_cast<std::uint16_t>(left_target - left_amount) };
    return step >= 100 ? '9' : step >= 10 ? '6' : '3';
  }
  if (row == RESOURCES - 1)
    return ENTER;
  latch = UNMOVED;
  return '2';
}

// The services of the transfer panel, its step and the supply orders, as direct calls; `module` names the caller in
// diagnostics.
uint16_t svc_transfer(uint8_t* ds, const char* module, uint16_t id,
                      uint16_t* args, uint16_t count) {
  std::uint16_t r{ 0 };
  switch (static_cast<TransferService>(id)) {
    case MOUSE_SHOW: opeu_mouse_show(ds, svc_mouse); break;
    case INPUT_DELAY:
      opeu_delay_ticks(ds, svc_waitkey, Arg(args, count, 0));
      break;
    case MOUSE_POSITION:
      // The mouse routine writes x and y to two words on the DS stack; they go back to the caller's words.
      WithStack(4, [&](std::uint16_t sp) {
        auto const at{ static_cast<std::uint16_t>(sp - 4) };
        opeu_mouse_position(ds, svc_mouse, at,
                            static_cast<std::uint16_t>(at + 2));
        args[0] = Word(ds, at);
        args[1] = Word(ds, static_cast<std::uint16_t>(at + 2));
      });
      break;
    case RIGHT_DOWN: r = (uint16_t)opeu_mouse_right(ds, svc_mouse); break;
    case LEFT_DOWN:  r = (uint16_t)opeu_mouse_left(ds, svc_mouse); break;
    case MOUSE_HIDE: opeu_mouse_hide(ds, svc_mouse); break;
    case KEY:        r = opeu_keyboard_key(ds, svc_keyboard); break;
    case RESTORE:
      r = (uint16_t)opeu_bitmap_restore(
          Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2),
          Arg(args, count, 3), Arg(args, count, 4),
          port_far(Arg(args, count, 5), Arg(args, count, 6)));
      break;
    case POSITION:
      opeu_army_position(ds, (TextHost*)port_textout_host(),
                         (int16_t)Arg(args, count, 0),
                         (int16_t)Arg(args, count, 1));
      break;
    case PRINT: r = lf_cursor_print(ds, args, count); break;
    case GLYPH:
      r = (uint16_t)opeu_vga_glyph(Arg(args, count, 0), Arg(args, count, 1),
                                   Arg(args, count, 2), Arg(args, count, 3),
                                   Arg(args, count, 4),
                                   PORT_DS(Arg(args, count, 5)));
      break;
    case TITLE:
      opeu_army_title(ds, (const DialogHost*)port_dialogs_host(),
                      Arg(args, count, 0));
      break;
    case MAP_DIALOG: opeu_ems_map_dialog(ds, svc_emsmap); break;
    case BOX:
      r = (uint16_t)opeu_ui_box(Arg(args, count, 0), Arg(args, count, 1),
                                Arg(args, count, 2), Arg(args, count, 3),
                                Arg(args, count, 4));
      break;
    case SOUND_CUE_14:    opeu_sound_cue14(ds, svc_soundcue); break;
    case SOUND_CUE_16:    opeu_sound_cue16(ds, svc_soundcue); break;
    case MUSIC_COMMAND_7: opeu_music_command7(ds, svc_sound); break;
    case PENDING_FINISH:
      opeu_pending_finish(ds, svc_waitkey, (uint8_t)Arg(args, count, 0));
      break;
    case DAILY_CONTEXT:
      opeu_daily_context(ds, svc_context, (uint8_t)Arg(args, count, 0));
      break;
    case CONFIRM:
      r = opeu_confirm_ok(ds, (const DialogHost*)port_dialogs_host(),
                          Arg(args, count, 0));
      break;
    case INPUT_RESET: opeu_input_reset(ds, svc_mouse); break;
    case HIGHLIGHT:
      r = (uint16_t)opeu_ui_highlight(Arg(args, count, 0), Arg(args, count, 1),
                                      Arg(args, count, 2), Arg(args, count, 3),
                                      Arg(args, count, 4));
      break;
    case UNIT_REPORT:
      opeu_unit_report(ds, (const DialogHost*)port_dialogs_host(),
                       Arg(args, count, 0), Arg(args, count, 1));
      break;
    case UNIT_ELIGIBLE:
      r = opeu_unit_selection_eligible(ds, Arg(args, count, 0));
      break;
    case DAILY_SHOW:
      opeu_daily_show(ds, (const DialogHost*)port_dialogs_host(),
                      Arg(args, count, 0), (uint8_t)Arg(args, count, 1),
                      (unsigned int)((uint32_t)Arg(args, count, 3) << 16 |
                                     Arg(args, count, 2)));
      break;
    case UNIT_CAPACITIES:
      opeu_unit_capacities(ds, Arg(args, count, 0),
                           port_ds_word_ptr(Arg(args, count, 1)),
                           port_ds_word_ptr(Arg(args, count, 2)),
                           port_ds_word_ptr(Arg(args, count, 3)));
      break;
    case OBJECTIVE_CAPACITIES:
      opeu_objective_capacities(ds, Arg(args, count, 0),
                                port_ds_word_ptr(Arg(args, count, 1)),
                                port_ds_word_ptr(Arg(args, count, 2)),
                                port_ds_word_ptr(Arg(args, count, 3)));
      break;
    case SIDE_PERCENTAGES:
      opeu_side_resource_percentages(ds, (uint8_t)Arg(args, count, 0),
                                     port_ds_word_ptr(Arg(args, count, 1)),
                                     port_ds_word_ptr(Arg(args, count, 2)),
                                     port_ds_word_ptr(Arg(args, count, 3)),
                                     Arg(args, count, 4));
      break;
    case UNIT_SLOT:  r = opeu_unit_index(Arg(args, count, 0)); break;
    case LAST_SLOT:  r = opeu_category_7((uint8_t)Arg(args, count, 0)); break;
    case UNIT_GROUP: r = opeu_unit_group(Arg(args, count, 0)); break;
    case UNIT_NAME:
      r = opeu_unit_name(ds, svc_unitname, Arg(args, count, 0),
                         Arg(args, count, 1));
      break;
    case UNIT_TRAINING: r = opeu_unit_average10(ds, Arg(args, count, 0)); break;
    case UNIT_EXPERIENCE:
      r = opeu_unit_average11(ds, Arg(args, count, 0));
      break;
    case OBJECTIVE_TARGET:
      r = opeu_move_objective_target(ds, svc_movetgt, Arg(args, count, 0),
                                     Arg(args, count, 1),
                                     (uint8_t)Arg(args, count, 2));
      break;
    case UNIT_TARGET:
      r = WithStack(SERVICE_STACK, [&](std::uint16_t sp) {
        return opeu_move_unit_target(ds, svc_movetgt,
                                     static_cast<std::uint16_t>(sp - 2),
                                     Arg(args, count, 0), Arg(args, count, 1));
      });
      break;
    case ADJACENT:
      r = WithStack(SERVICE_STACK, [&](std::uint16_t sp) {
        return opeu_move_adjacent(
            ds, svc_movetgt, static_cast<std::uint16_t>(sp - 2),
            static_cast<std::uint8_t>(Arg(args, count, 0)),
            static_cast<std::uint8_t>(Arg(args, count, 1)));
      });
      break;
    case ORDER_PROGRESS:
      r = opeu_move_level_lower(ds, Arg(args, count, 0),
                                (uint8_t)Arg(args, count, 1));
      break;
    case RESTORE_ORDER: opeu_restore_unit_order(ds, Arg(args, count, 0)); break;
    default:            port_unhandled(module, id, count);
  }
  return r;
}

uint16_t svc_xferpnl(uint8_t* ds, uint16_t id, uint16_t* args, uint16_t count) {
  return svc_transfer(ds, "xferpnl", id, args, count);
}
