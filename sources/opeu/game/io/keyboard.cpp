// Keyboard waits, the blinking text cursor, decimal entry, value choice and yes/no prompts.
#include "io/keyboard.hpp"

#include "common/data-segment.hpp"
#include "common/dispatch.hpp"
#include "common/game.hpp"
#include "common/keys.hpp"
#include "common/memory.hpp"
#include "common/menu.hpp"
#include "core/limits.hpp"
#include "core/memory.hpp"
#include "core/textout.hpp"
#include "io/busy-wait.hpp"
#include "io/menudraw.hpp"
#include "io/mouse.hpp"
#include "io/numdlg.hpp"
#include "io/waitkey.hpp"
#include "port.hpp"
#include "services.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <ranges>
#include <span>
#include <string>

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

// The idle hooks, by the enumerator idle_hook holds (NONE never runs).
constexpr auto IDLE_HOOKS = DispatchRows<IdleHook, auto (*)(Game&)->void>({
    { IdleHook::TEXT_CURSOR, opeu_text_cursor_draw },
});

auto HookCursor(DataSegment& data) -> void {
  data.idle_hook = IdleHook::TEXT_CURSOR;
}

auto HookClear(DataSegment& data) -> void {
  data.idle_hook = IdleHook::NONE;
}

// The value of typed digits, in 32-bit unsigned arithmetic that wraps.
auto DecimalValue(std::span<std::uint8_t const> digits) -> std::uint32_t {
  return std::ranges::fold_left(digits, std::uint32_t{ 0 }, [](std::uint32_t value, std::uint8_t digit) { return value * 10 + (digit - '0'); });
}

// The idle hook, if one is set.
auto CallHook(Game& game) -> void {
  auto const hook{ Data(game).idle_hook };
  if (hook != IdleHook::NONE) Dispatch(IDLE_HOOKS, hook, game);
}

auto TextX(Game const& game) -> std::uint16_t {
  return static_cast<std::uint16_t>(Data(game).text_x);
}

auto PreviousItem(std::uint16_t item) -> std::uint16_t {
  return static_cast<std::uint16_t>(item - sizeof(MenuItem));
}

// Calls the idle hook around timed waits until input arrives.
auto IdleUntilInput(Game& game) -> void {
  WaitUntil([&] {
    CallHook(game);
    opeu_key_wait_tick(game);
    CallHook(game);
    return opeu_key_wait_tick(game) != 0;
  });
}

// Erases the typed characters back to column `start`; each erase moves the cursor one column back.
auto EraseBackTo(Game& game, std::uint16_t start) -> void {
  while (start < TextX(game)) opeu_text_erase_char(game);
}

// A keyboard walk through a menu's items: the item lit and the value it stands for.
struct MenuCursor {
  std::uint16_t item;
  std::uint16_t value;
};

// The next item, wrapping from the last value to the first item.
auto NextChoice(MenuCursor cursor, ValueRange const& range, std::uint16_t first) -> MenuCursor {
  cursor.item = NextMenuItem(cursor.item);
  if (++cursor.value > range.hi) return { first, range.lo };
  return cursor;
}

// The previous item, wrapping from the first value to the last item.
auto PreviousChoice(MenuCursor const& cursor, ValueRange const& range) -> MenuCursor {
  if (cursor.value > range.lo) return { PreviousItem(cursor.item), static_cast<std::uint16_t>(cursor.value - 1) };
  return { static_cast<std::uint16_t>(cursor.item + (range.hi - cursor.value) * sizeof(MenuItem)), range.hi };
}

// Chooses from the menu `first` with the keypad: 2/6 next, 8/4 previous, Enter accepts, ESC or 0 cancel
// (MENU_CANCELLED). Leaves the menu drawn.
auto MenuByKeys(Game& game, ValueRange const& range, std::uint16_t first) -> std::uint16_t {
  opeu_menu_draw(game, first);
  MenuCursor cursor{ first, range.lo };
  opeu_menu_draw_item(game, cursor.item, MenuLight::LIT);
  for (;;) {
    WaitUntil([&] { return game.Machine().Input().KeyWaiting(); });
    switch (static_cast<std::uint8_t>(opeu_key_read(game))) {
    case '2':
    case '6':
      opeu_menu_draw_item(game, cursor.item, MenuLight::PLAIN);
      cursor = NextChoice(cursor, range, first);
      break;
    case '8':
    case '4':
      opeu_menu_draw_item(game, cursor.item, MenuLight::PLAIN);
      cursor = PreviousChoice(cursor, range);
      break;
    case KEY_ENTER:  opeu_menu_draw_item(game, cursor.item, MenuLight::LIT); return cursor.value;
    case KEY_ESCAPE:
    case '0':        return MENU_CANCELLED;
    default:         continue;
    }
    opeu_menu_draw_item(game, cursor.item, MenuLight::LIT);
  }
}

// Decimal entry in a field as wide as the range's top value, repeated until in range; an empty entry cancels
// (MENU_CANCELLED) only when allowed.
auto DigitsInRange(Game& game, ValueRange const& range, int cancel) -> std::uint16_t {
  auto const start{ TextX(game) };
  opeu_text_set_y(game, static_cast<std::int16_t>(Data(game).text_y));
  auto const width{ static_cast<std::uint16_t>(std::to_string(range.hi).size()) };
  std::ranges::for_each(std::views::iota(0u, std::uint32_t{ width }), [&](std::uint32_t) { opeu_text_put(game, static_cast<std::uint8_t>(' ')); });
  EraseBackTo(game, start);
  for (;;) {
    auto const value{ opeu_read_decimal(game, width) };
    if (value >= range.lo && value <= range.hi) return static_cast<std::uint16_t>(value);
    if (value == -1 && cancel) return MENU_CANCELLED;
    EraseBackTo(game, start);
  }
}

// What the yes/no pointer covers and where it was drawn: the saved graphics' 8-pixel column and row, the pointer's
// pixel position; 0xffff for none.
struct YesNoPointer {
  ScreenPoint saved{ 0xffff, 0xffff };
  ScreenPoint drawn{ 0xffff, 0xffff };
};

// Points the picture stream at one of the pointer's pictures.
auto PointAtPicture(DataSegment& data, std::uint16_t offset) -> void {
  SetStream(data, POINTER_SEGMENT, offset);
}

auto RestoreUnder(Game& game, ScreenPoint const& saved) -> void {
  auto& data{ Data(game) };
  PointAtPicture(data, POINTER_SAVE);
  game.Machine().Display().DrawColumns({ saved.x, saved.y, 5, 48 }, { 0, FarBytes(game, data.stream_ptr[1], data.stream_ptr[0]) });
}

auto SaveUnder(Game& game, ScreenPoint const& saved) -> void {
  auto& data{ Data(game) };
  PointAtPicture(data, POINTER_SAVE);
  game.Machine().Display().SaveColumns({ saved.x, saved.y, 5, 48 }, FarBytes(game, data.stream_ptr[1], data.stream_ptr[0]));
}

auto DrawPointer(Game& game, ScreenPoint const& at) -> void {
  auto& data{ Data(game) };
  PointAtPicture(data, POINTER_SHAPE);
  game.Machine().Display().ComposeShiftedSprite({ at.x, at.y, 32, 48 }, { .and_mask = FarBytes(game, data.stream_ptr[1], data.stream_ptr[0]),
                                                                          .or_image = FarBytes(game, POINTER_SEGMENT, POINTER_MASK),
                                                                          .buffer   = FarBytes(game, data.numdlg_save_segment, 0),
                                                                          .flags    = 1 });
}

// Follows the mouse with the pointer: restores the graphics it leaves, saves those it covers, draws it where it moved.
auto FollowMouse(Game& game, YesNoPointer& pointer) -> void {
  auto const mouse{ MousePosition(game) };
  bool const moved{ mouse.x != pointer.drawn.x || mouse.y != pointer.drawn.y };
  if (pointer.saved.x != 0xffff && moved) RestoreUnder(game, pointer.saved);
  if (mouse.x >> 3 != pointer.saved.x || mouse.y != pointer.saved.y) {
    pointer.saved = { static_cast<std::uint16_t>(mouse.x >> 3), mouse.y };
    SaveUnder(game, pointer.saved);
  }
  if (!moved) return;
  DrawPointer(game, mouse);
  pointer.drawn = mouse;
  opeu_delay_ticks(game, 3);
}

// 'Y' for a left press, 'N' for a right one, else 0.
auto Answer(std::uint8_t buttons) -> std::uint8_t {
  if (buttons & 1) return 'Y';
  if (buttons & 2) return 'N';
  return 0;
}
} // namespace

// The blinking cursor tile; installed as the idle hook.
void opeu_text_cursor_draw(Game& game) {
  auto& data{ Data(game) };
  SetStream(data, POINTER_SEGMENT, CURSOR_TILE);
  game.Machine().Display().DrawBits({ opeu_min16(data.text_x, 0x4e), static_cast<uint16_t>(data.text_y), 2, 16 }, { INVERT_MODE, FarBytes(game, data.stream_ptr[1], data.stream_ptr[0]) });
}

// Polls input (mouse buttons or a pending key) while a 15-tick timer runs; on input reseeds the random sequence with
// the advanced count and returns 1, else returns 0. The loop counts its polls.
auto opeu_key_wait_tick(Game& game) -> int {
  std::uint16_t count{ game.State().RandomNext() };
  game.Machine().Sound().StartCountdown(15);
  do {
    count++;
    if (HasMouse(Data(game)) ? opeu_turn_input_a(game) : game.Machine().Input().KeyWaiting()) {
      game.State().RandomSeed(count);
      return 1;
    }
  } while (game.Machine().Sound().CountdownLeft());
  return 0;
}

// Flushes, then calls the idle hook around timed waits until input arrives, and reads the key.
auto opeu_keyboard_key(Game& game) -> std::uint8_t {
  opeu_input_flush(game);
  IdleUntilInput(game);
  return static_cast<std::uint8_t>(opeu_key_read(game));
}

// As opeu_keyboard_key, for the mouse buttons.
auto opeu_mouse_wait_buttons(Game& game) -> std::uint8_t {
  opeu_input_reset(game);
  IdleUntilInput(game);
  return opeu_mouse_buttons(game);
}

// Without human sides just delays; else waits for a button or Enter with the blinking cursor.
void opeu_wait_confirm(Game& game) {
  auto& data{ Data(game) };
  if (!data.side_control[0] && !data.side_control[1]) {
    opeu_delay_units(game, 20);
    return;
  }
  HookCursor(data);
  WaitUntil([&] { return HasMouse(Data(game)) ? opeu_mouse_wait_buttons(game) != 0 : opeu_keyboard_key(game) == KEY_ENTER; });
  HookClear(data);
}

// Reads up to `digits` digits with echo; DEL, BS and left erase, Enter or ESC ends. Returns the value, or -1 when
// nothing was typed.
auto opeu_read_decimal(Game& game, std::uint16_t digits) -> std::int32_t {
  auto&                                data{ Data(game) };
  std::array<std::uint8_t, MAX_DIGITS> typed_digits{};
  std::uint8_t                         typed{ 0 };
  for (;;) {
    HookCursor(data);
    std::uint8_t const key{ opeu_keyboard_key(game) };
    HookClear(data);
    if (key >= '0' && key <= '9') {
      if (typed < digits) {
        typed_digits.at(typed++) = key;
        opeu_text_put(game, key);
      }
    } else
      switch (key) {
      case DELETE:
      case BACKSPACE:
      case LEFT_ARROW:
        if (typed) {
          opeu_text_erase_char(game);
          typed_digits.at(--typed) = ' ';
        }
        break;
      case KEY_ENTER:
      case KEY_ESCAPE:
        if (typed) return static_cast<std::int32_t>(DecimalValue(std::span{ typed_digits }.first(typed)));
        return -1;
      }
  }
}

// Chooses a value in the range. With a mouse: the menu `items`, else the numeric dialog. From the keyboard with a menu
// (unless the range ends at 24): the keypad walks the menu; otherwise decimal entry. Answers MENU_CANCELLED when
// cancelled.
auto opeu_choose_value(Game& game, ValueRange range, ChoiceOptions options) -> std::uint16_t {
  auto const [cancel, items]{ options };
  if (range.hi < range.lo) return 0;
  if (HasMouse(Data(game))) {
    if (items) return opeu_menu_select(game, items, cancel);
    return opeu_numdlg_run(game, range, cancel);
  }
  auto const value{ items && range.hi != 24 ? MenuByKeys(game, range, items) : DigitsInRange(game, range, cancel) };
  if (items) opeu_menu_restore(game, items);
  return value;
}

// Prints the prompt `text`, then chooses.
auto opeu_prompt_value(Game& game, std::uint16_t text, ValueChoice const& choice) -> std::uint16_t {
  auto const& data{ Data(game) };
  opeu_cursor_print(game, std::array<FormatArgument, 2>{ Near(data, data.prompt_format), text });
  return opeu_choose_value(game, choice.range, choice.options);
}

auto opeu_prompt_value_cancel(Game& game, std::uint16_t text, std::uint16_t max, std::uint16_t items) -> std::uint16_t {
  return opeu_prompt_value(game, text,
                           {
                               .range = { 1,           max            },
                                 .options = { .cancel = 1, .items = items }
  });
}

auto opeu_prompt_value_required(Game& game, std::uint16_t text, std::uint16_t max, std::uint16_t items) -> std::uint16_t {
  return opeu_prompt_value(game, text,
                           {
                               .range = { 1,           max            },
                                 .options = { .cancel = 0, .items = items }
  });
}

// The yes/no pointer: limits the mouse, keeps the 5x48 graphics under it saved, draws the pointer when it moves; a
// left press answers 'Y', a right press 'N'. The loop polls until a press.
auto opeu_mouse_yes_no(Game& game) -> std::uint8_t {
  YesNoPointer pointer;
  std::uint8_t answer{ 0 };
  opeu_input_reset(game);
  LimitMouse(game, { .x = 0, .y = 0, .width = 599, .height = 351 });
  while (answer == 0) {
    FollowMouse(game, pointer);
    answer = Answer(opeu_mouse_buttons(game));
  }
  RestoreUnder(game, pointer.saved);
  LimitMouse(game, { .x = 0, .y = 0, .width = 639, .height = 399 });
  return answer;
}

// y, Y and Enter answer 1; n, N, ESC and 0 answer 0; other keys are ignored.
auto opeu_yes_no(Game& game) -> int {
  auto& data{ Data(game) };
  HookCursor(data);
  for (;;) switch (HasMouse(Data(game)) ? opeu_mouse_yes_no(game) : opeu_keyboard_key(game)) {
    case 'y':
    case KEY_ENTER:
    case 'Y':        HookClear(data); return 1;
    case '0':
    case KEY_ESCAPE:
    case 'N':
    case 'n':        HookClear(data); return 0;
    }
}

auto opeu_prompt_yes_no(Game& game, std::uint16_t text) -> int {
  auto const& data{ Data(game) };
  opeu_cursor_print(game, std::array<FormatArgument, 2>{ Near(data, data.question_format), text });
  return opeu_yes_no(game);
}
