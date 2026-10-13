// The list selection dialogs on a list window (io/listui). Flags: FIRST_ACTIVATION resets the selection,
// CANCEL_ALLOWED lets the player cancel, SAVE_SCREEN saves and restores the screen under the window, KEYBOARD_ONLY
// hides the mouse scroll arrows. The window's select routine receives the running choice count (or count+10 to
// withdraw the last choice in the commander dialog) and returns how many were added.
#include "io/listdlg.hpp"
#include "core/textout.hpp"
#include "io/keyboard.hpp"
#include "io/mouse.hpp"
#include "link/glue.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "io/list-keys.hpp"
#include "io/listui.hpp"
#include "port.hpp"
#include "vgalib.hpp"
#include <array>

using namespace opeu::game;

namespace {

enum Flag : std::uint8_t {
  FIRST_ACTIVATION = 1,
  CANCEL_ALLOWED   = 2,
  SAVE_SCREEN      = 4,
  KEYBOARD_ONLY    = 8,
};

constexpr std::uint8_t  WITHDRAW{ 10 };
constexpr std::uint8_t  COMMANDERS{ 7 };
constexpr std::uint16_t ROW_HEIGHT{ 0x11 };

auto SaveScreen(Game& game, ListWindow const& window) -> void {
  opeu_bitmap_save(static_cast<std::uint16_t>(window.x), static_cast<std::uint16_t>(window.y), static_cast<std::uint16_t>(window.right - window.x + 1),
                   static_cast<std::uint16_t>(window.bottom - window.y + 1), port_far(static_cast<std::uint16_t>(Data(game).bitmap_buffer), 0));
}

auto RestoreScreen(Game& game, ListWindow const& window) -> void {
  opeu_bitmap_restore(static_cast<std::uint16_t>(window.x), static_cast<std::uint16_t>(window.y), static_cast<std::uint16_t>(window.right - window.x + 1),
                      static_cast<std::uint16_t>(window.bottom - window.y + 1), 0, port_far(static_cast<std::uint16_t>(Data(game).bitmap_buffer), 0));
}

auto Select(Game& game, HostWordsEvent event, ListWindow const& window, std::uint8_t count) -> std::uint8_t {
  return static_cast<std::uint8_t>(List(game, event, ListCall::SELECT, window.select.offset, window.select.segment, count));
}

auto NextKey(Game& game, HostWordsEvent event) -> std::uint8_t {
  return Data(game).input_mode ? opeu_list_mouse_key(game, event) : static_cast<std::uint8_t>(opeu_keyboard_key(game));
}

// Opens the dialog on the window: saves the screen when asked, draws the window (from the origin on the first
// activation) and inverts the selected row.
auto Open(Game& game, HostWordsEvent event, std::uint16_t near, std::uint8_t flags) -> void {
  auto&       data{ Data(game) };
  auto const& window{ ListWindowAt(data, near) };
  data.list_mouse = !(flags & KEYBOARD_ONLY);
  if (flags & SAVE_SCREEN) SaveScreen(game, window);
  std::uint8_t column;
  if (flags & FIRST_ACTIVATION) {
    data.list_window = near;
    data.list_row    = 0;
    column           = REDRAW_ORIGIN;
  } else
    column = REDRAW_HERE;
  opeu_list_scroll(game, event, near, column, 0);
  opeu_list_invert(game);
}
} // namespace

// The commander dialog: keeps the picked generals (general_list entries) and, after the seventh pick, asks "Are you
// satisfied with the Div 4 Commander selection?"; a refusal withdraws the last pick. Ignores `limit`.
auto opeu_commander_list_dialog(Game& game, HostWordsEvent event, std::uint16_t near, std::uint8_t, std::uint8_t flags) -> std::uint16_t {
  auto&         data{ Data(game) };
  auto const&   window{ ListWindowAt(data, near) };
  std::uint16_t picked[10]{};
  std::uint16_t result{ 0 };
  Open(game, event, near, flags);
  std::uint8_t const column{ data.list_column };
  std::uint8_t       start{ data.list_top };
  std::uint8_t       used{ 0 };
  bool               refresh{ true };
  for (;;) {
    if (refresh) {
      opeu_input_reset(game);
      refresh = false;
    }
    bool         scroll{ true };
    std::uint8_t key{ NextKey(game, event) };
    bool         withdraw{ false };
    switch (key) {
    case KEY_SCROLL_UP:
      if (data.list_top == 0) continue;
      opeu_list_invert(game);
      start--;
      data.list_row--;
      break;
    case KEY_SCROLL_DOWN:
      if (data.list_top + data.list_rows + 1 >= window.rows) continue;
      opeu_list_invert(game);
      start++;
      data.list_row++;
      break;
    case KEY_UP:
    case KEY_UP_5:
      if (data.list_row == 0) continue;
      opeu_list_invert(game);
      if (data.list_row-- != data.list_top)
        scroll = false;
      else
        start--;
      break;
    case KEY_DOWN:
      if (window.rows <= data.list_row + 2) continue;
      opeu_list_invert(game);
      data.list_row++;
      if (data.list_row - data.list_top + 1 <= data.list_rows)
        scroll = false;
      else
        start++;
      break;
    case KEY_ENTER: {
      auto const added{ Select(game, event, window, used) };
      used += added;
      if ((flags & FIRST_ACTIVATION) && added) picked[used] = data.general_list[data.list_row];
      opeu_list_invert(game);
      refresh = true;
      break;
    }
    case KEY_ESCAPE:
    case KEY_ZERO:   withdraw = true; break;
    default:         continue;
    }
    for (;;) {
      if (withdraw) {
        // withdraws the last pick and reprints its general's line in the list
        auto const general{ picked[used] };
        used -= Select(game, event, window, static_cast<std::uint8_t>(used + WITHDRAW));
        std::uint16_t i{ 0 };
        while (data.general_list[i] != general && i <= 11) i++;
        std::uint16_t const screen_y = (i - data.list_top) * ROW_HEIGHT + window.y + 1;
        if (screen_y > 0xe8 && screen_y < 0x140) {
          auto const& person{ Record<Person>(data, general) };
          opeu_text_color(game, static_cast<TextHost*>(port_textout_host()), 7);
          opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<std::int16_t>(static_cast<std::uint16_t>(Byte(game, window.widths) + window.x - 0x1c)),
                             static_cast<std::int16_t>(static_cast<std::uint16_t>((i - data.list_top) * ROW_HEIGHT + window.y + 0x12)));
          opeu_cursor_print(game, std::array<std::uint16_t, 8>{ static_cast<std::uint16_t>(Near(data, data.general_stats_line)), static_cast<std::uint16_t>(person.leadership),
                                                                static_cast<std::uint16_t>(person.charm), static_cast<std::uint16_t>(person.bravery), static_cast<std::uint16_t>(person.health),
                                                                static_cast<std::uint16_t>(person.infantry), static_cast<std::uint16_t>(person.artillery), static_cast<std::uint16_t>(person.armor) });
        }
        opeu_list_invert(game);
        refresh = true;
        if (used > 6) used = 6;
        withdraw = false;
        scroll   = true;
      }
      if (scroll) opeu_list_scroll(game, event, near, column, start);
      opeu_list_invert(game);
      if (used > COMMANDERS) used = COMMANDERS;
      if (used == COMMANDERS) {
        opeu_text_paper_set(game, static_cast<TextHost*>(port_textout_host()), static_cast<std::uint8_t>(4));
        for (std::uint16_t y{ 0x170 }; y < 0x188; y += 0x10)
          for (std::uint16_t x{ 0 }; x < 0x50; x += 2) opeu_screen_copy(0, 0, 2, 0x10, x, y);
        opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<std::int16_t>(0x15), static_cast<std::int16_t>(0x170));
        if (!static_cast<std::uint16_t>(opeu_prompt_yes_no(game, static_cast<std::uint16_t>(Near(data, data.div_commander_prompt))))) {
          used     = COMMANDERS;
          withdraw = true;
          continue;
        }
        opeu_text_paper_set(game, static_cast<TextHost*>(port_textout_host()), static_cast<std::uint8_t>(0));
        result = 1;
        if (flags & SAVE_SCREEN) RestoreScreen(game, window);
        return result;
      }
      if (used > 6) used = 6;
      break;
    }
  }
}

// The list dialog: runs until `limit` choices are made (or the player cancels when allowed). On a window at column
// 12h OK cancels and ESC selects: the dialog tests (flags == 1 | 2), which always holds. Windows at columns 0Fh and
// 9 restore the screen and start over when another key ends the switch.
auto opeu_list_dialog(Game& game, HostWordsEvent event, std::uint16_t near, std::uint8_t limit, std::uint8_t flags) -> std::uint16_t {
  auto&                   data{ Data(game) };
  auto const&             window{ ListWindowAt(data, near) };
  constexpr std::uint16_t SWAPPED_COLUMN{ 0x12 };
  for (;;) {
    Open(game, event, near, flags);
    std::uint8_t const column{ data.list_column };
    std::uint8_t       first{ data.list_top };
    std::uint8_t       count{ 0 };
    bool               fresh{ true };
    std::uint16_t      done;
    bool               finished{ false };
    for (;;) {
      if (fresh) {
        opeu_input_reset(game);
        fresh = false;
      }
      // what the key leads to
      enum class Step {
        NONE,
        SCROLL,
        MOVED,
        SELECT,
        CANCEL
      } step{ Step::NONE };
      switch (NextKey(game, event)) {
      case KEY_SCROLL_UP:
        if (data.list_top == 0) continue;
        opeu_list_invert(game);
        first--;
        data.list_row--;
        step = Step::SCROLL;
        break;
      case KEY_SCROLL_DOWN:
        if (data.list_top + data.list_rows + 1 >= window.rows) continue;
        opeu_list_invert(game);
        first++;
        data.list_row++;
        step = Step::SCROLL;
        break;
      case KEY_UP:
      case KEY_UP_5:
        if (data.list_row == 0) continue;
        opeu_list_invert(game);
        if (data.list_row-- != data.list_top)
          step = Step::MOVED;
        else {
          first--;
          step = Step::SCROLL;
        }
        break;
      case KEY_DOWN:
        if (window.rows <= data.list_row + 2) continue;
        opeu_list_invert(game);
        data.list_row++;
        if (data.list_row - data.list_top + 1 <= data.list_rows)
          step = Step::MOVED;
        else {
          first++;
          step = Step::SCROLL;
        }
        break;
      case KEY_ENTER:  step = window.x == SWAPPED_COLUMN ? Step::CANCEL : Step::SELECT; break;
      case KEY_ESCAPE:
      case KEY_ZERO:   step = window.x == SWAPPED_COLUMN ? Step::SELECT : Step::CANCEL; break;
      }
      if (step == Step::SELECT) {
        count += Select(game, event, window, count);
        opeu_list_invert(game);
        fresh = true;
        step  = Step::SCROLL;
      }
      if (step == Step::SCROLL) {
        opeu_list_scroll(game, event, near, column, first);
        step = Step::MOVED;
      }
      if (step == Step::MOVED) {
        opeu_list_invert(game);
        if (count >= limit) {
          done     = 1;
          finished = true;
          break;
        }
        continue;
      }
      if (step == Step::CANCEL && (flags & CANCEL_ALLOWED)) {
        done     = 0;
        finished = true;
        break;
      }
      if (window.x == 0xf || window.x == 9) {
        RestoreScreen(game, window);
        opeu_input_reset(game);
        break; // start over
      }
    }
    if (finished) {
      if (flags & SAVE_SCREEN) RestoreScreen(game, window);
      return done;
    }
  }
}

auto svc_listdlg(Game& game, std::uint16_t kind, std::uint16_t* args, std::uint16_t count) -> std::uint16_t {
  return svc_listui(game, kind, args, count);
}
