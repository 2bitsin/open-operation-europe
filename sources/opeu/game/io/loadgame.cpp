// The load menu and the saved-game checksum.
#include "io/loadgame.hpp"

#include "common/data-segment.hpp"
#include "common/dos-file.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "common/saved-game.hpp"
#include "common/text.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "io/listdlg.hpp"
#include "io/rebase.hpp"
#include "io/resopen.hpp"
#include "io/waitkey.hpp"
#include "port.hpp"
#include "services.hpp"

#include <algorithm>
#include <array>

using namespace opeu::game;

namespace {

constexpr std::uint8_t HIGHLIGHT{ 4 };
constexpr std::uint8_t NORMAL{ 0 };

// Where a framed message goes: an 8-pixel column and a pixel row.
struct Place {
  std::uint16_t x;
  std::uint16_t y;
};

constexpr Place MESSAGE{ 32, 188 };
constexpr Place QUESTION{ 33, 336 };
constexpr Place RESULT{ 29, 188 };

auto Framed(Game& game, Place const& place, auto const& text) -> std::uint16_t {
  return opeu_title_box(game, place.x, place.y, Near(Data(game), text));
}

auto Paper(Game& game, std::uint8_t paper) -> void {
  opeu_text_paper_set(game, paper);
}

// Prints a framed message on the highlighted paper.
auto Say(Game& game, Place const& place, auto const& text) -> void {
  opeu_ui_text(game, Framed(game, place, text));
}

auto NoSavedGames(Game& game) -> void {
  auto const& data{ Data(game) };
  opeu_cursor_print(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(Near(data, data.load_print_format)), Framed(game, MESSAGE, data.no_saved_games_text) });
  opeu_delay_units(game, 23);
  Paper(game, NORMAL);
}

// Reads the 20 slot headers.
auto ReadSlotHeaders(Game& game, std::int16_t file) -> void {
  auto& data{ Data(game) };
  std::ranges::for_each(data.save_slots, [&](SaveSlot& slot) { game.Machine().Files().Read(HandleOf(file), game.State().Bytes(Near(data, slot), sizeof slot)); });
}

// Loads and rebases the chosen slot's game; answers whether it is valid.
auto LoadChosenSlot(Game& game, std::int16_t file) -> bool {
  auto&      data{ Data(game) };
  auto const position{ static_cast<std::uint32_t>(SAVED_BYTES * data.save_slot + SAVED_BLOCKS_AT) };
  game.Machine().Files().Seek(HandleOf(file), static_cast<std::int32_t>(position), services::SeekOrigin::START);
  game.Machine().Files().Read(HandleOf(file), game.State().Bytes(SAVED_FIRST, SAVED_BYTES));
  opeu_rebase_scenario(game, 1);
  auto const& slot{ data.save_slots[data.save_slot] };
  return opeu_saved_game_valid(game, data.save_slot) && slot.scenario != SLOT_EMPTY;
}

// Asks which slot to load and loads it; answers the menu choice, or 0 when nothing valid was loaded.
auto ChooseAndLoad(Game& game, std::int16_t file) -> std::uint16_t {
  auto const& data{ Data(game) };
  ReadSlotHeaders(game, file);
  Paper(game, HIGHLIGHT);
  Say(game, QUESTION, data.load_which_text);
  Paper(game, NORMAL);
  auto const choice{ opeu_list_dialog(game, data.save_list, { 1, 3 }) };
  if (choice == 0) return 0;
  Paper(game, HIGHLIGHT);
  Say(game, MESSAGE, data.loading_text);
  auto const valid{ LoadChosenSlot(game, file) };
  if (valid)
    Say(game, RESULT, data.loaded_text);
  else
    Say(game, RESULT, data.load_failed_text);
  opeu_delay_units(game, 15);
  Paper(game, NORMAL);
  return valid ? choice : 0;
}
} // namespace

// 1 when the byte sum of the saved part of the data segment equals the slot's checksum.
auto opeu_saved_game_valid(Game& game, std::uint8_t slot) -> std::uint16_t {
  auto const& data{ Data(game) };
  return data.save_slots[slot].checksum == SavedChecksum(data) ? 1 : 0;
}

// The load menu: reads the 20 slot headers from SAVEDATA, lets the player choose a slot, loads and rebases that
// game and reports whether it is valid. Returns the menu choice, or 0 when nothing valid was loaded.
auto opeu_load_saved_game(Game& game) -> std::uint16_t {
  auto const&   data{ Data(game) };
  std::uint16_t choice{ 0 };
  Paper(game, HIGHLIGHT);
  if (!game.Machine().Files().Exists(CString(data, Near(data, data.savedata_name)))) {
    NoSavedGames(game);
  } else {
    Paper(game, NORMAL);
    auto const file{ opeu_open_resource(game, Near(data, data.load_savedata_name)) };
    choice = ChooseAndLoad(game, file);
    game.Machine().Files().Close(HandleOf(file));
  }
  opeu_backdrop_fill_color(game, static_cast<std::uint8_t>(1));
  return choice;
}
