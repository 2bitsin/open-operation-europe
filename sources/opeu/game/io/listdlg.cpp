// The list selection dialogs on a list window (io/listui). Flags: FIRST_ACTIVATION resets the selection,
// CANCEL_ALLOWED lets the player cancel, SAVE_SCREEN saves and restores the screen under the window, KEYBOARD_ONLY
// hides the mouse scroll arrows. The window's select routine receives the running choice count (or count+10 to
// withdraw the last choice in the commander dialog) and returns how many were added.
#include "io/listdlg.hpp"

#include "common/data-segment.hpp"
#include "io/list-keys.hpp"
#include "io/listui.hpp"
#include "port.hpp"

using namespace opeu::game;

namespace {

enum Flag : std::uint8_t {
  FIRST_ACTIVATION = 1,
  CANCEL_ALLOWED   = 2,
  SAVE_SCREEN      = 4,
  KEYBOARD_ONLY    = 8,
};

constexpr std::uint8_t  REDRAW_ORIGIN{ 0xff };
constexpr std::uint8_t  REDRAW_HERE{ 0xfe };
constexpr std::uint8_t  WITHDRAW{ 10 };
constexpr std::uint8_t  COMMANDERS{ 7 };
constexpr std::uint16_t ROW_HEIGHT{ 0x11 };

auto Window(DataSegment& data, std::uint16_t near) -> ListWindow& {
  return Record<ListWindow>(data, near);
}

auto SaveScreen(std::uint8_t* ds, HostWordsEvent event, ListWindow const& window) -> void {
  List(ds, event, ListCall::BITMAP_SAVE, window.x, window.y, static_cast<std::uint16_t>(window.right - window.x + 1), static_cast<std::uint16_t>(window.bottom - window.y + 1), Data(ds).bitmap_buffer,
       0);
}

auto RestoreScreen(std::uint8_t* ds, HostWordsEvent event, ListWindow const& window) -> void {
  List(ds, event, ListCall::BITMAP_RESTORE, window.x, window.y, static_cast<std::uint16_t>(window.right - window.x + 1), static_cast<std::uint16_t>(window.bottom - window.y + 1), 0,
       Data(ds).bitmap_buffer, 0);
}

auto Select(std::uint8_t* ds, HostWordsEvent event, ListWindow const& window, std::uint8_t count) -> std::uint8_t {
  return static_cast<std::uint8_t>(List(ds, event, ListCall::SELECT, window.select.offset, window.select.segment, count));
}

auto NextKey(std::uint8_t* ds, HostWordsEvent event) -> std::uint8_t {
  return Data(ds).input_mode ? opeu_list_mouse_key(ds, event) : static_cast<std::uint8_t>(List(ds, event, ListCall::KEYBOARD_KEY));
}

// Opens the dialog on the window: saves the screen when asked, draws the window (from the origin on the first
// activation) and inverts the selected row.
auto Open(std::uint8_t* ds, HostWordsEvent event, std::uint16_t near, std::uint8_t flags) -> void {
  auto&       data{ Data(ds) };
  auto const& window{ Window(data, near) };
  data.list_mouse = !(flags & KEYBOARD_ONLY);
  if (flags & SAVE_SCREEN) SaveScreen(ds, event, window);
  std::uint8_t column;
  if (flags & FIRST_ACTIVATION) {
    data.list_window = near;
    data.list_row    = 0;
    column           = REDRAW_ORIGIN;
  } else
    column = REDRAW_HERE;
  opeu_list_scroll(ds, event, near, column, 0);
  opeu_list_invert(ds, event);
}
} // namespace

// The commander dialog: keeps the picked generals (general_list entries) and, after the seventh pick, asks "Are you
// satisfied with the Div 4 Commander selection?"; a refusal withdraws the last pick. Ignores `limit`.
auto opeu_commander_list_dialog(std::uint8_t* ds, HostWordsEvent event, std::uint16_t near, std::uint8_t, std::uint8_t flags) -> std::uint16_t {
  auto&         data{ Data(ds) };
  auto const&   window{ Window(data, near) };
  std::uint16_t picked[10]{};
  std::uint16_t result{ 0 };
  Open(ds, event, near, flags);
  std::uint8_t const column{ data.list_column };
  std::uint8_t       start{ data.list_top };
  std::uint8_t       used{ 0 };
  bool               refresh{ true };
  for (;;) {
    if (refresh) {
      List(ds, event, ListCall::INPUT_RESET);
      refresh = false;
    }
    bool         scroll{ true };
    std::uint8_t key{ NextKey(ds, event) };
    bool         withdraw{ false };
    switch (key) {
    case KEY_SCROLL_UP:
      if (data.list_top == 0) continue;
      opeu_list_invert(ds, event);
      start--;
      data.list_row--;
      break;
    case KEY_SCROLL_DOWN:
      if (data.list_top + data.list_rows + 1 >= window.rows) continue;
      opeu_list_invert(ds, event);
      start++;
      data.list_row++;
      break;
    case KEY_UP:
    case KEY_UP_5:
      if (data.list_row == 0) continue;
      opeu_list_invert(ds, event);
      if (data.list_row-- != data.list_top)
        scroll = false;
      else
        start--;
      break;
    case KEY_DOWN:
      if (window.rows <= data.list_row + 2) continue;
      opeu_list_invert(ds, event);
      data.list_row++;
      if (data.list_row - data.list_top + 1 <= data.list_rows)
        scroll = false;
      else
        start++;
      break;
    case KEY_ENTER: {
      auto const added{ Select(ds, event, window, used) };
      used += added;
      if ((flags & FIRST_ACTIVATION) && added) picked[used] = data.general_list[data.list_row];
      opeu_list_invert(ds, event);
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
        used -= Select(ds, event, window, static_cast<std::uint8_t>(used + WITHDRAW));
        std::uint16_t i{ 0 };
        while (data.general_list[i] != general && i <= 11) i++;
        std::uint16_t const screen_y = (i - data.list_top) * ROW_HEIGHT + window.y + 1;
        if (screen_y > 0xe8 && screen_y < 0x140) {
          auto const& person{ Record<Person>(data, general) };
          List(ds, event, ListCall::TEXT_COLOUR, 7);
          List(ds, event, ListCall::TEXT_POSITION, ds[window.widths] + window.x - 0x1c, (i - data.list_top) * ROW_HEIGHT + window.y + 0x12);
          List(ds, event, ListCall::PRINT, Near(data, data.general_stats_line), person.leadership, person.charm, person.bravery, person.health, person.infantry, person.artillery, person.armor);
        }
        opeu_list_invert(ds, event);
        refresh = true;
        if (used > 6) used = 6;
        withdraw = false;
        scroll   = true;
      }
      if (scroll) opeu_list_scroll(ds, event, near, column, start);
      opeu_list_invert(ds, event);
      if (used > COMMANDERS) used = COMMANDERS;
      if (used == COMMANDERS) {
        List(ds, event, ListCall::PAPER, 4);
        for (std::uint16_t y{ 0x170 }; y < 0x188; y += 0x10)
          for (std::uint16_t x{ 0 }; x < 0x50; x += 2) List(ds, event, ListCall::SCREEN_COPY, 0, 0, 2, 0x10, x, y);
        List(ds, event, ListCall::TEXT_POSITION, 0x15, 0x170);
        if (!List(ds, event, ListCall::YES_NO, Near(data, data.div_commander_prompt))) {
          used     = COMMANDERS;
          withdraw = true;
          continue;
        }
        List(ds, event, ListCall::PAPER, 0);
        result = 1;
        if (flags & SAVE_SCREEN) RestoreScreen(ds, event, window);
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
auto opeu_list_dialog(std::uint8_t* ds, HostWordsEvent event, std::uint16_t near, std::uint8_t limit, std::uint8_t flags) -> std::uint16_t {
  auto&                   data{ Data(ds) };
  auto const&             window{ Window(data, near) };
  constexpr std::uint16_t SWAPPED_COLUMN{ 0x12 };
  for (;;) {
    Open(ds, event, near, flags);
    std::uint8_t const column{ data.list_column };
    std::uint8_t       first{ data.list_top };
    std::uint8_t       count{ 0 };
    bool               fresh{ true };
    std::uint16_t      done;
    bool               finished{ false };
    for (;;) {
      if (fresh) {
        List(ds, event, ListCall::INPUT_RESET);
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
      switch (NextKey(ds, event)) {
      case KEY_SCROLL_UP:
        if (data.list_top == 0) continue;
        opeu_list_invert(ds, event);
        first--;
        data.list_row--;
        step = Step::SCROLL;
        break;
      case KEY_SCROLL_DOWN:
        if (data.list_top + data.list_rows + 1 >= window.rows) continue;
        opeu_list_invert(ds, event);
        first++;
        data.list_row++;
        step = Step::SCROLL;
        break;
      case KEY_UP:
      case KEY_UP_5:
        if (data.list_row == 0) continue;
        opeu_list_invert(ds, event);
        if (data.list_row-- != data.list_top)
          step = Step::MOVED;
        else {
          first--;
          step = Step::SCROLL;
        }
        break;
      case KEY_DOWN:
        if (window.rows <= data.list_row + 2) continue;
        opeu_list_invert(ds, event);
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
        count += Select(ds, event, window, count);
        opeu_list_invert(ds, event);
        fresh = true;
        step  = Step::SCROLL;
      }
      if (step == Step::SCROLL) {
        opeu_list_scroll(ds, event, near, column, first);
        step = Step::MOVED;
      }
      if (step == Step::MOVED) {
        opeu_list_invert(ds, event);
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
        RestoreScreen(ds, event, window);
        List(ds, event, ListCall::INPUT_RESET);
        break; // start over
      }
    }
    if (finished) {
      if (flags & SAVE_SCREEN) RestoreScreen(ds, event, window);
      return done;
    }
  }
}

auto svc_listdlg(std::uint8_t* ds, std::uint16_t kind, std::uint16_t* args, std::uint16_t count) -> std::uint16_t {
  return svc_listui(ds, kind, args, count);
}

// The game passes the window last.
auto port_routine_opeu_list_dialog(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return opeu_list_dialog(ds, svc_listdlg, Arg(args, count, 2), static_cast<std::uint8_t>(Arg(args, count, 0)), static_cast<std::uint8_t>(Arg(args, count, 1)));
}
