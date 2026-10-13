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
#include "common/dispatch.hpp"
#include "common/game.hpp"
#include "core/airdrop.hpp"
#include "core/persmenu.hpp"
#include "core/rqsupply.hpp"
#include "core/staffgen.hpp"
#include "core/stockord.hpp"
#include "core/suptable.hpp"
#include "io/list-keys.hpp"
#include "io/listui.hpp"
#include "io/tblsave.hpp"
#include "map/dplrows.hpp"
#include "map/ordtgt.hpp"
#include "port.hpp"
#include "units/cmdxfer.hpp"
#include "units/genpick.hpp"
#include "units/ordunit.hpp"
#include "units/rqarms.hpp"
#include "units/tally.hpp"
#include "units/vwlist.hpp"
#include "vgalib.hpp"

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <ranges>
#include <span>

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

// A select routine called with the game and the running choice count, whichever of them it takes.
template <auto SELECT>
auto Selecting(Game& game, std::uint8_t count) -> std::uint8_t {
  if constexpr (std::invocable<decltype(SELECT), Game&, std::uint8_t>)
    return SELECT(game, count);
  else if constexpr (std::invocable<decltype(SELECT), Game&>)
    return SELECT(game);
  else
    return SELECT();
}

// The select routines, by the enumerator a window's select slot holds. NONE is absent: a window without one is never
// run as a dialog.
constexpr auto SELECTS = DispatchRows<ListSelect, auto (*)(Game&, std::uint8_t)->std::uint8_t>({
    { ListSelect::REROLL_STAFF,         Selecting<opeu_staff_reroll_selected>    },
    { ListSelect::ASSIGN_GENERAL,       Selecting<opeu_general_assign_choice>    },
    { ListSelect::SAVE_ACCEPT,          Selecting<opeu_tbl_save_accept>          },
    { ListSelect::GROUP_TARGET,         Selecting<opeu_group_target_enabled>     },
    { ListSelect::ANY_ORDER_TARGET,     Selecting<opeu_menu_always>              },
    { ListSelect::SUPPLY_UNIT,          Selecting<opeu_supply_unit_enabled>      },
    { ListSelect::STOCK_CITY,           Selecting<opeu_stock_city_enabled>       },
    { ListSelect::TRANSFER_GROUP,       Selecting<opeu_transfer_group_enabled>   },
    { ListSelect::TRANSFER_UNIT,        Selecting<opeu_transfer_unit_enabled>    },
    { ListSelect::TRANSFER_ELEMENT,     Selecting<opeu_transfer_element_enabled> },
    { ListSelect::ANY_GENERAL,          Selecting<opeu_commander_always>         },
    { ListSelect::DIVISION_TOGGLE,      Selecting<opeu_division_toggle>          },
    { ListSelect::VIEW_OBJECTIVE,       Selecting<opeu_view_objective_ok>        },
    { ListSelect::VIEW_NONE,            Selecting<opeu_view_false>               },
    { ListSelect::VIEW_DIVISION,        Selecting<opeu_view_group_ok>            },
    { ListSelect::VIEW_REGIMENT,        Selecting<opeu_view_unit_ok>             },
    { ListSelect::VIEW_NONE_2,          Selecting<opeu_view_false_2>             },
    { ListSelect::VIEW_NONE_3,          Selecting<opeu_view_false_3>             },
    { ListSelect::VIEW_ANY,             Selecting<opeu_view_true>                },
    { ListSelect::SUPPLY_UNIT_ELIGIBLE, Selecting<opeu_supply_unit_eligible>     },
    { ListSelect::ANY_SUPPLY_TYPE,      Selecting<opeu_supply_true>              },
    { ListSelect::RETURN_SLOT,          Selecting<opeu_supply_return_slot>       },
    { ListSelect::TALLY_MARK,           Selecting<opeu_tally_mark>               },
    { ListSelect::REQUEST_TYPE,         Selecting<opeu_rq_type_selectable>       },
    { ListSelect::REQUEST_UNIT,         Selecting<opeu_rq_unit_selectable>       },
    { ListSelect::AIR_MARK_REGIMENT,    Selecting<opeu_air_mark_regiment>        },
    { ListSelect::AIR_ASSIGN_TROOPS,    Selecting<opeu_air_assign_troops>        },
    { ListSelect::AIR_MARK_BATTALION,   Selecting<opeu_air_mark_battalion>       },
    { ListSelect::ANY_DEPLOYMENT,       Selecting<opeu_deploy_true>              },
});

auto Select(Game& game, ListWindow const& window, std::uint8_t count) -> std::uint8_t {
  return Dispatch(SELECTS, ListSelect{ window.select }, game, count);
}

auto NextKey(Game& game) -> std::uint8_t {
  return Data(game).input_mode ? opeu_list_mouse_key(game) : static_cast<std::uint8_t>(opeu_keyboard_key(game));
}

// Opens the dialog on the window: saves the screen when asked, draws the window (from the origin on the first
// activation) and inverts the selected row.
auto Open(Game& game, ListWindow const& window, std::uint8_t flags) -> void {
  auto& data{ Data(game) };
  data.list_mouse = !(flags & KEYBOARD_ONLY);
  if (flags & SAVE_SCREEN) SaveScreen(game, window);
  std::uint8_t column{ REDRAW_HERE };
  if (flags & FIRST_ACTIVATION) {
    data.list_window = Near(data, window);
    data.list_row    = 0;
    column           = REDRAW_ORIGIN;
  }
  opeu_list_scroll(game, window, column, 0);
  opeu_list_invert(game);
}

// The two-column cells of a prompt line.
constexpr std::uint32_t PROMPT_CELLS{ 0x28 };

// The rows general_list holds for the commander dialog.
constexpr std::size_t LISTED_GENERALS{ 12 };

// Reprints a withdrawn general's line in the list, when it is on the screen.
auto ReprintGeneral(Game& game, ListWindow const& window, std::uint16_t general) -> void {
  auto&               data{ Data(game) };
  auto const          listed{ std::span{ data.general_list }.first(LISTED_GENERALS) };
  auto const          i{ static_cast<std::uint16_t>(std::ranges::find(listed, general) - listed.begin()) };
  std::uint16_t const screen_y = (i - data.list_top) * ROW_HEIGHT + window.y + 1;
  if (screen_y <= 0xe8 || screen_y >= 0x140) return;
  auto const& person{ Record<Person>(data, general) };
  opeu_text_color(game, 7);
  opeu_army_position(game, static_cast<std::int16_t>(static_cast<std::uint16_t>(Byte(game, window.widths) + window.x - 0x1c)),
                     static_cast<std::int16_t>(static_cast<std::uint16_t>((i - data.list_top) * ROW_HEIGHT + window.y + 0x12)));
  opeu_cursor_print(game, std::array<std::uint16_t, 8>{ static_cast<std::uint16_t>(Near(data, data.general_stats_line)), static_cast<std::uint16_t>(person.leadership),
                                                        static_cast<std::uint16_t>(person.charm), static_cast<std::uint16_t>(person.bravery), static_cast<std::uint16_t>(person.health),
                                                        static_cast<std::uint16_t>(person.infantry), static_cast<std::uint16_t>(person.artillery), static_cast<std::uint16_t>(person.armor) });
}

// Clears the prompt line and asks "Are you satisfied with the Div 4 Commander selection?"; the paper stays highlighted
// after a refusal.
auto CommandersConfirmed(Game& game) -> bool {
  auto const& data{ Data(game) };
  opeu_text_paper_set(game, static_cast<std::uint8_t>(4));
  // two text rows of 40 two-column cells, row by row, each a copy of the top-left cell
  std::ranges::for_each(std::views::iota(0u, 2u * PROMPT_CELLS), [](std::uint32_t cell) {
    opeu_screen_copy(0, 0, 2, 0x10, static_cast<std::uint16_t>(cell % PROMPT_CELLS * 2), static_cast<std::uint16_t>(0x170 + cell / PROMPT_CELLS * 0x10));
  });
  opeu_army_position(game, static_cast<std::int16_t>(0x15), static_cast<std::int16_t>(0x170));
  if (!static_cast<std::uint16_t>(opeu_prompt_yes_no(game, static_cast<std::uint16_t>(Near(data, data.div_commander_prompt))))) return false;
  opeu_text_paper_set(game, static_cast<std::uint8_t>(0));
  return true;
}
} // namespace

// The commander dialog: keeps the picked generals (general_list entries) and, after the seventh pick, asks "Are you
// satisfied with the Div 4 Commander selection?"; a refusal withdraws the last pick. Ignores `limit`.
auto opeu_commander_list_dialog(Game& game, std::uint16_t near, std::uint8_t, std::uint8_t flags) -> std::uint16_t {
  auto&                         data{ Data(game) };
  auto const&                   window{ ListWindowAt(data, near) };
  std::array<std::uint16_t, 10> picked{};
  Open(game, window, flags);
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
    std::uint8_t key{ NextKey(game) };
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
      auto const added{ Select(game, window, used) };
      used += added;
      if ((flags & FIRST_ACTIVATION) && added) picked.at(used) = data.general_list[data.list_row];
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
        auto const general{ picked.at(used) };
        used -= Select(game, window, static_cast<std::uint8_t>(used + WITHDRAW));
        ReprintGeneral(game, window, general);
        opeu_list_invert(game);
        refresh = true;
        if (used > 6) used = 6;
        withdraw = false;
        scroll   = true;
      }
      if (scroll) opeu_list_scroll(game, window, column, start);
      opeu_list_invert(game);
      used = std::min(used, COMMANDERS);
      if (used != COMMANDERS) break;
      if (!CommandersConfirmed(game)) {
        withdraw = true;
        continue;
      }
      if (flags & SAVE_SCREEN) RestoreScreen(game, window);
      return 1;
    }
  }
}

// The list dialog: runs until `limit` choices are made (or the player cancels when allowed). On a window at column
// 12h OK cancels and ESC selects: the dialog tests (flags == 1 | 2), which always holds. Windows at columns 0Fh and
// 9 restore the screen and start over when another key ends the switch.
auto opeu_list_dialog(Game& game, std::uint16_t near, std::uint8_t limit, std::uint8_t flags) -> std::uint16_t {
  auto&                   data{ Data(game) };
  auto const&             window{ ListWindowAt(data, near) };
  constexpr std::uint16_t SWAPPED_COLUMN{ 0x12 };
  for (;;) {
    Open(game, window, flags);
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
      switch (NextKey(game)) {
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
        count += Select(game, window, count);
        opeu_list_invert(game);
        fresh = true;
        step  = Step::SCROLL;
      }
      if (step == Step::SCROLL) {
        opeu_list_scroll(game, window, column, first);
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
