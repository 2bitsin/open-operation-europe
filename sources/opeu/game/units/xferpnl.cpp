// The supply-transfer panel of the supply orders: the keypad drawn on it, mouse and keyboard input, the transfer
// table (per side and resource: the amount, the capacity and the computer's target), the names in its title, the
// rows with their fill percentages and the computer's keys. A twin of the supply-allocation panel (core/suppanel).
#include "units/xferpnl.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/exchange-panel.hpp"
#include "common/keys.hpp"
#include "common/memory.hpp"
#include "core/strings.hpp"
#include "types.hpp"
#include "units/ratio.hpp"
#include "units/transfer-service.hpp"

#include "common/game.hpp"
#include "core/context.hpp"
#include "core/dialogs.hpp"
#include "core/recindex.hpp"
#include "core/textout.hpp"
#include "io/keyboard.hpp"
#include "io/mouse.hpp"
#include "io/sound.hpp"
#include "io/soundcue.hpp"
#include "io/waitkey.hpp"
#include "link/glue.hpp"
#include "map/movetgt.hpp"
#include "map/support.hpp"
#include "port.hpp"
#include "services.hpp"
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

// The keys of the keypad's columns, top to bottom.
constexpr std::uint8_t KEYS[3][3]{
  { '7', '4', '1' },
  { '8', '5', '2' },
  { '9', '6', '3' }
};

constexpr std::uint16_t BOX_WIDTH{ 3 }; // the percentage's background, in 8-pixel columns
constexpr std::uint16_t BOX_HEIGHT{ 6 };

// The computer's direction latch: which way it last moved supplies, so that it never moves them back.
constexpr std::uint8_t TOWARD_LEFT{ '<' };
constexpr std::uint8_t TOWARD_RIGHT{ '>' };
constexpr std::uint8_t UNMOVED{ '-' };

auto MousePosition(Game& game, HostWordsEvent event) -> std::array<std::uint16_t, 2> {
  std::array<std::uint16_t, 2> position{};
  event(game, std::to_underlying(MOUSE_POSITION), position.data(), 0);
  return position;
}
} // namespace

// The key under a point of the keypad, NO_KEY off it.
uint8_t opeu_transfer_keypad_hit(Game& game, uint16_t x, uint16_t y) {
  auto const&  data{ Data(game) };
  auto const   left{ static_cast<std::uint16_t>(data.transfer_panel_x << 3) };
  auto const   top{ data.transfer_panel_y };
  std::uint8_t key{ NO_KEY };
  for (auto column = 0u; column < 3; ++column) {
    if (static_cast<std::uint16_t>(left + KEY_COLUMNS[column]) > x || static_cast<std::uint16_t>(left + KEY_COLUMNS[column + 1]) <= x) continue;
    for (auto row = 0u; row < 3; ++row)
      if (static_cast<std::uint16_t>(top + KEY_ROWS[row]) <= y && static_cast<std::uint16_t>(top + KEY_ROWS[row + 1]) > y) key = KEYS[column][row];
  }
  return key;
}

// With the mouse: the right button is Enter and a left click on the keys 8, 5 or 2 selects row 0, 1 or 2 (stored at
// the near pointer row, NO_KEY returned). With the keyboard: the key.
uint8_t opeu_transfer_input(Game& game, HostWordsEvent event, uint16_t row) {
  auto& data{ Data(game) };
  if (!data.input_mode) return static_cast<std::uint8_t>(opeu_keyboard_key(game, svc_keyboard));
  opeu_mouse_show(game, svc_mouse);
  opeu_delay_ticks(game, 3);
  std::uint8_t key;
  for (;;) {
    auto const [x, y]{ MousePosition(game, event) };
    if (static_cast<uint16_t>(opeu_mouse_right(game, svc_mouse))) {
      key = KEY_ENTER;
      break;
    }
    if (static_cast<uint16_t>(opeu_mouse_left(game, svc_mouse))) {
      key = opeu_transfer_keypad_hit(game, x, y);
      for (std::uint8_t selected = 0; selected < RESOURCES; ++selected)
        if (key == KEYS[1][selected]) {
          Record<std::uint8_t>(data, row) = selected;
          key                             = NO_KEY;
        }
      break;
    }
  }
  opeu_mouse_hide(game, svc_mouse);
  return key;
}

void opeu_transfer_set(Game& game, uint8_t side, uint8_t row, uint8_t column, uint16_t value) {
  Data(game).transfer_table[side][row][column] = value;
}

uint16_t opeu_transfer_get(Game const& game, uint8_t side, uint8_t row, uint8_t column) {
  return Data(game).transfer_table[side][row][column];
}

// For a player's side: titles the panel, restores its background and centres the two names, at most ten characters
// each.
void opeu_transfer_draw(Game& game, uint16_t first, uint16_t second) {
  auto const& data{ Data(game) };
  if (!SidePlayed(data, data.current_side)) return;
  opeu_army_title(game, static_cast<const DialogHost*>(port_dialogs_host()), offsetof(DataSegment, transfer_title));
  opeu_bitmap_restore(data.transfer_panel_x, data.transfer_panel_y, PANEL_WIDTH, PANEL_HEIGHT, 0, port_far(data.numdlg_image_segment, PANEL_IMAGE_AT));
  auto const indent{ [&](std::uint16_t name) {
    auto const length{ std::min(static_cast<std::int16_t>(opeu_string_length(game, name)), NAME_LIMIT) };
    return static_cast<std::int16_t>(NAME_LIMIT - length) / 2;
  } };
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(indent(first) + data.transfer_panel_x + 2), static_cast<int16_t>(data.transfer_panel_y + 14));
  opeu_cursor_print(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(offsetof(DataSegment, transfer_name_format)), static_cast<std::uint16_t>(first) });
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(indent(second) + data.transfer_panel_x + 20), static_cast<int16_t>(data.transfer_panel_y + 14));
  opeu_cursor_print(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(offsetof(DataSegment, transfer_name_text)), static_cast<std::uint16_t>(second) });
}

// Draws value right-aligned leftwards from x as digit glyphs over blanks; zero draws one digit.
void opeu_transfer_digits(Game& game, HostWordsEvent event, uint16_t x, uint16_t y, uint16_t value) {
  DrawDigits(game, event, GLYPH, x, y, value);
}

// For a player's side, redraws a resource row: both amounts and both fill percentages. column (the column just
// changed) is not used.
void opeu_transfer_draw_row(Game& game, HostWordsEvent event, uint8_t row, uint8_t column) {
  (void)column;
  auto const& data{ Data(game) };
  if (!SidePlayed(data, data.current_side)) return;
  auto const& left{ data.transfer_table[0][row] };
  auto const& right{ data.transfer_table[1][row] };
  auto const  left_amount{ left[AMOUNT] };
  auto const  right_amount{ right[AMOUNT] };
  auto const  left_percent{ opeu_resource_percent(left_amount, left[CAPACITY]) };
  auto const  right_percent{ opeu_resource_percent(right_amount, right[CAPACITY]) };
  auto const  x{ data.transfer_panel_x };
  auto const  text_y{ row * PANEL_ROW_HEIGHT + data.transfer_panel_y + 0x24 };
  auto const  box_y{ static_cast<std::uint16_t>(row * PANEL_ROW_HEIGHT + data.transfer_panel_y + 0x37) };
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(x + 2), static_cast<int16_t>(text_y));
  opeu_cursor_print(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(offsetof(DataSegment, transfer_amount_format)), static_cast<std::uint16_t>(left_amount) });
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(x + 0x19), static_cast<int16_t>(text_y));
  opeu_cursor_print(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(offsetof(DataSegment, transfer_amount_text)), static_cast<std::uint16_t>(right_amount) });
  opeu_ui_box(x + 2, box_y, BOX_WIDTH, BOX_HEIGHT, 0);
  opeu_transfer_digits(game, event, static_cast<std::uint16_t>((x + 4) << 3), box_y, left_percent);
  opeu_ui_box(x + 0x19, box_y, BOX_WIDTH, BOX_HEIGHT, 0);
  opeu_transfer_digits(game, event, static_cast<std::uint16_t>((x + 0x1b) << 3), box_y, right_percent);
  opeu_cursor_print(game, std::array<std::uint16_t, 1>{ static_cast<std::uint16_t>(offsetof(DataSegment, transfer_colour)) });
}

// The computer's key for a resource row. Supplies move to the left side ('7', '4', '1') while its target exceeds its
// amount and the right side holds more than its target, or to the right side ('9', '6', '3') in the opposite case;
// the step compares the unsigned difference with 100 and 10 (so a move to the left is always '7'). The direction
// latch keeps the computer from moving supplies back. Otherwise '2' selects the next row, and Enter ends after the
// last one.
uint8_t opeu_transfer_auto_key(Game& game, uint8_t row) {
  auto&       data{ Data(game) };
  auto&       latch{ data.transfer_auto };
  auto const& left{ data.transfer_table[0][row] };
  auto const& right{ data.transfer_table[1][row] };
  auto const  left_amount{ left[AMOUNT] };
  auto const  right_amount{ right[AMOUNT] };
  auto const  left_target{ left[TARGET] };
  auto const  right_target{ right[TARGET] };
  if (latch != TOWARD_RIGHT && left_target > left_amount && right_target < right_amount) {
    latch = TOWARD_LEFT;
    auto const step{ static_cast<std::uint16_t>(right_target - right_amount) };
    return step >= 100 ? '7' : step >= 10 ? '4' : '1';
  }
  if (latch != TOWARD_LEFT && left_target < left_amount && right_target > right_amount) {
    latch = TOWARD_RIGHT;
    auto const step{ static_cast<std::uint16_t>(left_target - left_amount) };
    return step >= 100 ? '9' : step >= 10 ? '6' : '3';
  }
  if (row == RESOURCES - 1) return KEY_ENTER;
  latch = UNMOVED;
  return '2';
}

// The services of the transfer panel, its step and the supply orders, as direct calls; `module` names the caller in
// diagnostics.
uint16_t svc_transfer(Game& game, const char* module, uint16_t id, uint16_t* args, uint16_t count) {
  std::uint16_t r{ 0 };
  switch (static_cast<TransferService>(id)) {
  case MOUSE_SHOW:     opeu_mouse_show(game, svc_mouse); break;
  case INPUT_DELAY:    opeu_delay_ticks(game, Arg(args, count, 0)); break;
  case MOUSE_POSITION: {
    // x and y go back to the caller's words.
    auto const position{ MousePosition(game, svc_mouse) };
    args[0] = position.x;
    args[1] = position.y;
    break;
  }
  case RIGHT_DOWN: r = (uint16_t)opeu_mouse_right(game, svc_mouse); break;
  case LEFT_DOWN:  r = (uint16_t)opeu_mouse_left(game, svc_mouse); break;
  case MOUSE_HIDE: opeu_mouse_hide(game, svc_mouse); break;
  case KEY:        r = opeu_keyboard_key(game, svc_keyboard); break;
  case RESTORE:
    r = (uint16_t)opeu_bitmap_restore(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4), port_far(Arg(args, count, 5), Arg(args, count, 6)));
    break;
  case POSITION: opeu_army_position(game, (TextHost*)port_textout_host(), (int16_t)Arg(args, count, 0), (int16_t)Arg(args, count, 1)); break;
  case PRINT:    opeu_cursor_print(game, { args, count }); break;
  case GLYPH:
    r = (uint16_t)opeu_vga_glyph(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4), game.State().Bytes(Arg(args, count, 5)).data());
    break;
  case TITLE:           opeu_army_title(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0)); break;
  case BOX:             r = (uint16_t)opeu_ui_box(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4)); break;
  case SOUND_CUE_14:    opeu_sound_cue14(game, svc_soundcue); break;
  case SOUND_CUE_16:    opeu_sound_cue16(game, svc_soundcue); break;
  case MUSIC_COMMAND_7: opeu_music_command7(game, svc_sound); break;
  case PENDING_FINISH:  opeu_pending_finish(game, (uint8_t)Arg(args, count, 0)); break;
  case DAILY_CONTEXT:   opeu_daily_context(game, svc_context, (uint8_t)Arg(args, count, 0)); break;
  case CONFIRM:         r = opeu_confirm_ok(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0)); break;
  case INPUT_RESET:     opeu_input_reset(game, svc_mouse); break;
  case HIGHLIGHT:       r = (uint16_t)opeu_ui_highlight(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4)); break;
  case UNIT_REPORT:     opeu_unit_report(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case UNIT_ELIGIBLE:   r = opeu_unit_selection_eligible(game, Arg(args, count, 0)); break;
  case DAILY_SHOW:
    opeu_daily_show(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (unsigned int)((uint32_t)Arg(args, count, 3) << 16 | Arg(args, count, 2)));
    break;
  case UNIT_CAPACITIES:
    opeu_unit_capacities(game, Arg(args, count, 0), port_ds_word_ptr(game, Arg(args, count, 1)), port_ds_word_ptr(game, Arg(args, count, 2)), port_ds_word_ptr(game, Arg(args, count, 3)));
    break;
  case OBJECTIVE_CAPACITIES:
    opeu_objective_capacities(game, Arg(args, count, 0), port_ds_word_ptr(game, Arg(args, count, 1)), port_ds_word_ptr(game, Arg(args, count, 2)), port_ds_word_ptr(game, Arg(args, count, 3)));
    break;
  case SIDE_PERCENTAGES:
    opeu_side_resource_percentages(game, (uint8_t)Arg(args, count, 0), port_ds_word_ptr(game, Arg(args, count, 1)), port_ds_word_ptr(game, Arg(args, count, 2)),
                                   port_ds_word_ptr(game, Arg(args, count, 3)), Arg(args, count, 4));
    break;
  case UNIT_SLOT:        r = opeu_unit_index(Arg(args, count, 0)); break;
  case LAST_SLOT:        r = opeu_category_7((uint8_t)Arg(args, count, 0)); break;
  case UNIT_GROUP:       r = opeu_unit_group(Arg(args, count, 0)); break;
  case UNIT_NAME:        r = opeu_unit_name(game, svc_unitname, Arg(args, count, 0), Arg(args, count, 1)); break;
  case UNIT_TRAINING:    r = opeu_unit_average10(game, Arg(args, count, 0)); break;
  case UNIT_EXPERIENCE:  r = opeu_unit_average11(game, Arg(args, count, 0)); break;
  case OBJECTIVE_TARGET: r = opeu_move_objective_target(game, Arg(args, count, 0), Arg(args, count, 1), (uint8_t)Arg(args, count, 2)); break;
  case UNIT_TARGET:      r = opeu_move_unit_target(game, Arg(args, count, 0), Arg(args, count, 1)); break;
  case ADJACENT:         r = opeu_move_adjacent(game, static_cast<std::uint8_t>(Arg(args, count, 0)), static_cast<std::uint8_t>(Arg(args, count, 1))); break;
  case ORDER_PROGRESS:   r = opeu_move_level_lower(game, Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  case RESTORE_ORDER:    opeu_restore_unit_order(game, Arg(args, count, 0)); break;
  default:               port_unhandled(module, id, count);
  }
  return r;
}
