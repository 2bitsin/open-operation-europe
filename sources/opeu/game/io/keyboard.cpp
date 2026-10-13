// Keyboard waits, the blinking text cursor, decimal entry, value choice and yes/no prompts. The service is the shared
// input service (io/input-call.hpp), which this module also passes to the mouse, menu and numeric dialog routines it
// calls.
#include "io/keyboard.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/keys.hpp"
#include "common/memory.hpp"
#include "common/menu.hpp"
#include "common/random.hpp"
#include "core/limits.hpp"
#include "core/memory.hpp"
#include "core/textout.hpp"
#include "io/input-call.hpp"
#include "io/menudraw.hpp"
#include "io/mouse.hpp"
#include "io/numdlg.hpp"
#include "io/waitkey.hpp"
#include "link/glue.hpp"
#include "link/link.hpp"
#include "port.hpp"
#include "services.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <span>

using namespace opeu::game;

namespace {

constexpr std::uint8_t DELETE{ 0x7f };
constexpr std::uint8_t BACKSPACE{ 8 };
constexpr std::uint8_t LEFT_ARROW{ 0x1d };

constexpr std::uint16_t INVERT_MODE{ 0x107 };
constexpr std::size_t   MAX_DIGITS{ 16 }; // the typed digits a decimal entry keeps

// The pictures of the cursor and the yes/no pointer (data/fg3546): far segment and offsets.
constexpr std::uint16_t POINTER_SEGMENT{ 0x3546 };
constexpr std::uint16_t POINTER_SAVE{ 0x000 }; // the graphics under the pointer
constexpr std::uint16_t CURSOR_TILE{ 0x3c0 };
constexpr std::uint16_t POINTER_MASK{ 0x3e0 };
constexpr std::uint16_t POINTER_SHAPE{ 0x6e0 };

// The idle hook that blinks the cursor: opeu_text_cursor_draw's far address.
constexpr std::uint16_t CURSOR_HOOK_OFFSET{ 0x002e };
constexpr std::uint16_t CURSOR_HOOK_SEGMENT{ 0x0541 };

auto HookCursor(DataSegment& data) -> void {
  data.idle_hook = { CURSOR_HOOK_OFFSET, CURSOR_HOOK_SEGMENT };
}

auto HookClear(DataSegment& data) -> void {
  data.idle_hook = {};
}

// The value of typed digits, in 32-bit unsigned arithmetic that wraps.
auto DecimalValue(std::span<std::uint8_t const> digits) -> std::uint32_t {
  return std::ranges::fold_left(digits, std::uint32_t{ 0 }, [](std::uint32_t value, std::uint8_t digit) { return value * 10 + (digit - '0'); });
}

// The idle hook: the cursor routine, another far routine, or nothing.
auto CallHook(Game& game) -> void {
  auto const hook{ Data(game).idle_hook };
  if (!hook.offset && !hook.segment) return;
  if (hook.offset == CURSOR_HOOK_OFFSET && hook.segment == CURSOR_HOOK_SEGMENT)
    opeu_text_cursor_draw(game);
  else
    port_call(game, static_cast<std::uint32_t>(static_cast<std::uint16_t>(hook.segment)) * 16u + static_cast<std::uint16_t>(hook.offset), nullptr, 0);
}

auto TextX(Game const& game) -> std::uint16_t {
  return static_cast<std::uint16_t>(Data(game).text_x);
}

auto PreviousItem(std::uint16_t item) -> std::uint16_t {
  return static_cast<std::uint16_t>(item - sizeof(MenuItem));
}

} // namespace

// Inverts the text cursor cell (column at most 4fh).
void opeu_text_cursor_clear(Game& game, HostService service) {
  auto const& data{ Data(game) };
  Input(game, service, InputCall::CURSOR_INVERT, opeu_min16(data.text_x, 0x4f), data.text_y, 1, 16, INVERT_MODE);
}

// The blinking cursor tile; installed as the idle hook.
void opeu_text_cursor_draw(Game& game) {
  auto& data{ Data(game) };
  SetStream(data, POINTER_SEGMENT, CURSOR_TILE);
  opeu_draw_far(opeu_min16(data.text_x, 0x4e), data.text_y, 2, 16, INVERT_MODE, port_far(data.stream_ptr[1], data.stream_ptr[0]));
}

// Polls input (mouse buttons or a pending key) while a 15-tick timer runs; on input reseeds the random sequence with
// the advanced count and returns 1, else returns 0.
auto opeu_key_wait_tick(Game& game, HostService service) -> int {
  std::uint16_t count{ RandomNext(game) };
  opeu_fm_wait_start(15);
  do {
    count++;
    if (HasMouse(Data(game)) ? opeu_turn_input_a(game, service) : opeu_turn_input_b()) {
      RandomSeed(game, count);
      return 1;
    }
  } while (opeu_fm_busy());
  return 0;
}

// Flushes, then calls the idle hook around timed waits until input arrives, and reads the key.
auto opeu_keyboard_key(Game& game, HostService service) -> std::uint8_t {
  opeu_input_flush(game, static_cast<TextHost*>(port_textout_host()));
  do {
    CallHook(game);
    opeu_key_wait_tick(game, service);
    CallHook(game);
  } while (!opeu_key_wait_tick(game, service));
  return static_cast<std::uint8_t>(opeu_key_read(game, static_cast<TextHost*>(port_textout_host())));
}

// As opeu_keyboard_key, for the mouse buttons.
auto opeu_mouse_wait_buttons(Game& game, HostService service) -> std::uint8_t {
  opeu_input_reset(game, service);
  do {
    CallHook(game);
    opeu_key_wait_tick(game, service);
    CallHook(game);
  } while (!opeu_key_wait_tick(game, service));
  return opeu_mouse_buttons(game, service);
}

// Without human sides just delays; else waits for a button or Enter with the blinking cursor.
void opeu_wait_confirm(Game& game, HostService service, int pause) {
  auto& data{ Data(game) };
  if (!data.side_control[0] && !data.side_control[1]) {
    opeu_delay_units(game, static_cast<std::uint8_t>(20));
    return;
  }
  if (pause && !(data.side_control[1] | data.side_control[0])) {
    opeu_pending_finish(game, static_cast<std::uint8_t>(3));
    return;
  }
  HookCursor(data);
  while (!(HasMouse(Data(game)) ? opeu_mouse_wait_buttons(game, service) : opeu_keyboard_key(game, service) == KEY_ENTER));
  HookClear(data);
}

// Reads up to `digits` digits with echo; DEL, BS and left erase, Enter or ESC ends. Returns the value, or -1 when
// nothing was typed.
auto opeu_read_decimal(Game& game, HostService service, std::uint16_t digits) -> std::int32_t {
  auto&                                data{ Data(game) };
  std::array<std::uint8_t, MAX_DIGITS> typed_digits{};
  auto                                 digit{ [&](std::uint8_t n) -> std::uint8_t& { return typed_digits.at(n); } };
  std::uint8_t                         typed{ 0 };
  for (;;) {
    HookCursor(data);
    std::uint8_t const key{ opeu_keyboard_key(game, service) };
    HookClear(data);
    if (key >= '0' && key <= '9') {
      if (typed < digits) {
        digit(typed++) = key;
        opeu_text_put(game, static_cast<TextHost*>(port_textout_host()), static_cast<std::uint8_t>(key));
      }
    } else
      switch (key) {
      case DELETE:
      case BACKSPACE:
      case LEFT_ARROW:
        if (typed) {
          opeu_text_erase_char(game, static_cast<TextHost*>(port_textout_host()));
          digit(--typed) = ' ';
        }
        break;
      case KEY_ENTER:
      case KEY_ESCAPE:
        if (typed) return static_cast<std::int32_t>(DecimalValue(std::span{ typed_digits }.first(typed)));
        return -1;
      }
  }
}

// Chooses a value in min..max. With a mouse: the menu `items`, else the numeric dialog. From the keyboard with a menu
// (unless max is 24): keypad 2/6 next (wrapping to min), 8/4 previous (wrapping to max), Enter accepts, ESC or 0
// cancel (0xffff). Otherwise decimal entry in a field max's digits wide, repeated until in range; an empty entry
// cancels only when allowed.
auto opeu_choose_value(Game& game, HostService service, std::uint16_t min, std::uint16_t max, int cancel, std::uint16_t items) -> std::uint16_t {
  auto&        data{ Data(game) };
  std::int32_t value;
  if (max < min) return 0;
  if (HasMouse(Data(game))) {
    if (items) return opeu_menu_select(game, service, items, cancel);
    return opeu_numdlg_run(game, service, min, max, cancel);
  }
  if (items && max != 24) {
    opeu_menu_draw(game, items);
    std::uint16_t item{ items };
    opeu_menu_draw_item(game, item, 1);
    std::uint16_t chosen{ min };
    value = chosen;
    for (;;) {
      while (!opeu_turn_input_b());
      std::uint8_t const key{ static_cast<std::uint8_t>(opeu_key_read(game, static_cast<TextHost*>(port_textout_host()))) };
      switch (key) {
      case '2':
      case '6':
        opeu_menu_draw_item(game, item, 0);
        item = NextMenuItem(item);
        if (++chosen > max) {
          chosen = min;
          item   = items;
        }
        value = chosen;
        break;
      case '8':
      case '4':
        opeu_menu_draw_item(game, item, 0);
        if (chosen > min) {
          item = PreviousItem(item);
          chosen--;
        } else
          while (chosen < max) {
            chosen++;
            item = NextMenuItem(item);
          }
        value = chosen;
        break;
      case KEY_ENTER:  break;
      case KEY_ESCAPE:
      case '0':        opeu_menu_restore(game, items); return MENU_CANCELLED;
      default:         continue;
      }
      opeu_menu_draw_item(game, item, 1);
      if (key == KEY_ENTER || key == KEY_ESCAPE) break;
    }
  } else {
    auto const start{ TextX(game) };
    opeu_text_set_y(game, static_cast<TextHost*>(port_textout_host()), static_cast<std::int16_t>(data.text_y));
    std::uint16_t width{ 0 };
    for (std::uint16_t rest{ max };;) {
      opeu_text_put(game, static_cast<TextHost*>(port_textout_host()), static_cast<std::uint8_t>(' '));
      width++;
      if (!(rest /= 10)) break;
    }
    while (start < TextX(game)) opeu_text_erase_char(game, static_cast<TextHost*>(port_textout_host()));
    while ((value = opeu_read_decimal(game, service, width)) < min || value > max) {
      if (value == -1 && cancel) {
        if (items) opeu_menu_restore(game, items);
        return MENU_CANCELLED;
      }
      while (start < TextX(game)) opeu_text_erase_char(game, static_cast<TextHost*>(port_textout_host()));
    }
  }
  if (items) opeu_menu_restore(game, items);
  return static_cast<std::uint16_t>(value);
}

// Prints the prompt `text`, then chooses.
auto opeu_prompt_value(Game& game, HostService service, std::uint16_t text, std::uint16_t min, std::uint16_t max, int cancel, std::uint16_t items) -> std::uint16_t {
  auto const& data{ Data(game) };
  opeu_cursor_print(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(Near(data, data.prompt_format)), static_cast<std::uint16_t>(text) });
  return opeu_choose_value(game, service, min, max, cancel, items);
}

auto opeu_prompt_value_cancel(Game& game, HostService service, std::uint16_t text, std::uint16_t max, std::uint16_t items) -> std::uint16_t {
  return opeu_prompt_value(game, service, text, 1, max, 1, items);
}

auto opeu_prompt_value_required(Game& game, HostService service, std::uint16_t text, std::uint16_t max, std::uint16_t items) -> std::uint16_t {
  return opeu_prompt_value(game, service, text, 1, max, 0, items);
}

// The yes/no pointer: limits the mouse, keeps the 5x48 graphics under it saved, draws the pointer when it moves; a
// left press answers 'Y', a right press 'N'.
auto opeu_mouse_yes_no(Game& game, HostService service) -> std::uint8_t {
  auto&         data{ Data(game) };
  std::uint16_t saved_column{ 0xffff }; // where the saved graphics are, in 8-pixel columns and pixels
  std::uint16_t saved_y{ 0xffff };
  std::uint16_t drawn_x{ 0xffff }; // where the pointer was drawn
  std::uint16_t drawn_y{ 0xffff };
  std::uint8_t  answer;
  opeu_input_reset(game, service);
  opeu_mouse_window(game, service, 0, 0, 599, 351);
  for (;;) {
    auto const [x, y]{ MousePosition(game, service) };
    if (saved_column != 0xffff && (x != drawn_x || y != drawn_y)) {
      SetStream(data, POINTER_SEGMENT, POINTER_SAVE);
      opeu_bitmap_restore(saved_column, saved_y, 5, 48, 0, port_far(data.stream_ptr[1], data.stream_ptr[0]));
    }
    if (x >> 3 != saved_column || y != saved_y) {
      saved_column = x >> 3;
      saved_y      = y;
      SetStream(data, POINTER_SEGMENT, POINTER_SAVE);
      opeu_bitmap_save(saved_column, saved_y, 5, 48, port_far(data.stream_ptr[1], data.stream_ptr[0]));
    }
    if (x != drawn_x || y != drawn_y) {
      SetStream(data, POINTER_SEGMENT, POINTER_SHAPE);
      koei_3f26(x, y, 32, 48, port_far(data.stream_ptr[1], data.stream_ptr[0]), port_far(POINTER_SEGMENT, POINTER_MASK), port_far(data.numdlg_save_segment, 0), 1);
      drawn_x = x;
      drawn_y = y;
      opeu_delay_ticks(game, 3);
    }
    auto const buttons{ opeu_mouse_buttons(game, service) };
    if (buttons & 1) {
      answer = 'Y';
      break;
    }
    if (buttons & 2) {
      answer = 'N';
      break;
    }
  }
  SetStream(data, POINTER_SEGMENT, POINTER_SAVE);
  opeu_bitmap_restore(saved_column, saved_y, 5, 48, 0, port_far(data.stream_ptr[1], data.stream_ptr[0]));
  opeu_mouse_window(game, service, 0, 0, 639, 399);
  return answer;
}

// y, Y and Enter answer 1; n, N, ESC and 0 answer 0; other keys are ignored.
auto opeu_yes_no(Game& game, HostService service) -> int {
  auto& data{ Data(game) };
  HookCursor(data);
  for (;;) switch (HasMouse(Data(game)) ? opeu_mouse_yes_no(game, service) : opeu_keyboard_key(game, service)) {
    case 'y':
    case KEY_ENTER:
    case 'Y':        HookClear(data); return 1;
    case '0':
    case KEY_ESCAPE:
    case 'N':
    case 'n':        HookClear(data); return 0;
    }
}

auto opeu_prompt_yes_no(Game& game, HostService service, std::uint16_t text) -> int {
  auto const& data{ Data(game) };
  opeu_cursor_print(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(Near(data, data.question_format)), static_cast<std::uint16_t>(text) });
  return opeu_yes_no(game, service);
}

// The shared input service: the mouse, menu and numeric dialog calls go to their modules, the rest are served here.
auto svc_keyboard(Game& game, std::uint16_t call, std::uint16_t* w) -> std::uint16_t {
  auto* const text{ static_cast<TextHost*>(port_textout_host()) };
  switch (static_cast<InputCall>(call)) {
  case InputCall::MOUSE_INT86:    return svc_mouse(game, call, w);
  case InputCall::TEXT_SAVE:
  case InputCall::TEXT_POSITION:
  case InputCall::TEXT_PRINT:
  case InputCall::TEXT_RESTORE:
  case InputCall::TILE_BLIT:
  case InputCall::BITMAP_SAVE:
  case InputCall::BITMAP_RESTORE:
  case InputCall::DELAY_UNITS:    return svc_menudraw(game, call, w);
  case InputCall::UI_FILL:
  case InputCall::UI_INVERT:
  case InputCall::UI_LINE:        return svc_numdlg(game, call, w);
  case InputCall::READ_KEY:       return opeu_key_read(game, text);
  case InputCall::INPUT_FLUSH:    opeu_input_flush(game, text); break;
  case InputCall::TEXT_HOME:      opeu_text_set_y(game, text, static_cast<std::int16_t>(w[0])); break;
  case InputCall::TEXT_PUT:       opeu_text_put(game, text, static_cast<std::uint8_t>(w[0])); break;
  case InputCall::TEXT_BACKSPACE: opeu_text_erase_char(game, text); break;
  case InputCall::TIMER_START:    opeu_fm_wait_start(w[0]); break;
  case InputCall::TIMER_RUNNING:  return opeu_fm_busy();
  case InputCall::KEY_PENDING:    return opeu_turn_input_b();
  case InputCall::SPRITE_DRAW:    koei_3f26(w[0], w[1], w[2], w[3], port_far(w[5], w[4]), port_far(w[7], w[6]), port_far(w[9], w[8]), w[10]); break;
  case InputCall::DRAW_FAR:       opeu_draw_far(w[0], w[1], w[2], w[3], w[4], port_far(w[5], w[6])); break;
  case InputCall::CURSOR_INVERT:  opeu_ui_highlight(w[0], w[1], w[2], w[3], w[4]); break;
  case InputCall::DELAY_TICKS:    opeu_delay_ticks(game, w[0]); break;
  case InputCall::PENDING_FINISH: opeu_pending_finish(game, static_cast<std::uint8_t>(w[0])); break;
  case InputCall::PRINT:          opeu_cursor_print(game, { w, 2 }); break;
  case InputCall::IDLE_HOOK:      port_call(game, static_cast<std::uint32_t>(w[1]) * 16u + w[0], nullptr, 0); break;
  default:                        port_unhandled("keyboard", call, 0);
  }
  return 0;
}
