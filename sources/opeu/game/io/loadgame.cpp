// The load menu and the saved-game checksum.
#include "io/loadgame.hpp"

#include "common/data-segment.hpp"
#include "common/dos-file.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "common/saved-game.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "io/listdlg.hpp"
#include "io/rebase.hpp"
#include "io/resopen.hpp"
#include "io/waitkey.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"
#include <array>

using namespace opeu::game;

namespace {

constexpr std::uint16_t HIGHLIGHT{ 4 };
constexpr std::uint16_t NORMAL{ 0 };

auto Framed(Game& game, std::uint16_t x, std::uint16_t y, auto const& text) -> std::uint16_t {
  return opeu_title_box(game, static_cast<UiHost const*>(port_uiprim_host()), x, y, Near(Data(game), text));
}
} // namespace

// 1 when the byte sum of the saved part of the data segment equals the slot's checksum.
auto opeu_saved_game_valid(Game& game, std::uint8_t slot) -> std::uint16_t {
  auto const& data{ Data(game) };
  return data.save_slots[slot].checksum == SavedChecksum(data) ? 1 : 0;
}

auto opeu_save_record_size() -> std::uint16_t {
  return SAVED_BYTES;
}

// The load menu: reads the 20 slot headers from SAVEDATA, lets the player choose a slot, loads and rebases that
// game and reports whether it is valid. Returns the menu choice, or 0 when nothing valid was loaded.
auto opeu_load_saved_game(Game& game) -> std::uint16_t {
  auto&         data{ Data(game) };
  std::uint16_t choice{ 0 };
  opeu_text_paper_set(game, static_cast<TextHost*>(port_textout_host()), static_cast<std::uint8_t>(HIGHLIGHT));
  if (!static_cast<std::uint16_t>(opeu_dos_exists(reinterpret_cast<char const*>(game.State().Bytes(Near(data, data.savedata_name)).data())))) {
    opeu_cursor_print(game,
                      std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(Near(data, data.load_print_format)), static_cast<std::uint16_t>(Framed(game, 32, 188, data.no_saved_games_text)) });
    opeu_delay_units(game, static_cast<std::uint8_t>(23));
    opeu_text_paper_set(game, static_cast<TextHost*>(port_textout_host()), static_cast<std::uint8_t>(NORMAL));
  } else {
    opeu_text_paper_set(game, static_cast<TextHost*>(port_textout_host()), static_cast<std::uint8_t>(NORMAL));
    auto const file{ static_cast<std::uint16_t>(opeu_open_resource(game, Near(data, data.load_savedata_name))) };
    for (auto& slot : data.save_slots) opeu_dos_read(static_cast<std::int16_t>(file), game.State().Bytes(Near(data, slot), static_cast<std::uint16_t>(sizeof slot)).data(), sizeof slot);
    opeu_text_paper_set(game, static_cast<TextHost*>(port_textout_host()), static_cast<std::uint8_t>(HIGHLIGHT));
    opeu_ui_text(game, static_cast<TextHost*>(port_textout_host()), Framed(game, 33, 336, data.load_which_text));
    opeu_text_paper_set(game, static_cast<TextHost*>(port_textout_host()), static_cast<std::uint8_t>(NORMAL));
    if ((choice = opeu_list_dialog(game, svc_listdlg, Near(data, data.save_list), static_cast<std::uint8_t>(1), static_cast<std::uint8_t>(3))) != 0) {
      opeu_text_paper_set(game, static_cast<TextHost*>(port_textout_host()), static_cast<std::uint8_t>(HIGHLIGHT));
      opeu_ui_text(game, static_cast<TextHost*>(port_textout_host()), Framed(game, 32, 188, data.loading_text));
      auto const position{ static_cast<std::uint32_t>(SAVED_BYTES * data.save_slot + SAVED_BLOCKS_AT) };
      opeu_dos_seek(static_cast<std::int16_t>(file),
                    static_cast<std::int32_t>((static_cast<std::uint32_t>(static_cast<std::uint16_t>(static_cast<std::uint16_t>(position)))
                                               | static_cast<std::uint32_t>(static_cast<std::uint16_t>(static_cast<std::uint16_t>(position >> 16))) << 16)),
                    static_cast<std::int16_t>(SEEK_FROM_START));
      opeu_dos_read(static_cast<std::int16_t>(file), game.State().Bytes(SAVED_FIRST, SAVED_BYTES).data(), SAVED_BYTES);
      opeu_rebase_scenario(game, 1);
      auto const& slot{ data.save_slots[data.save_slot] };
      if (opeu_saved_game_valid(game, data.save_slot) && slot.scenario != SLOT_EMPTY)
        opeu_ui_text(game, static_cast<TextHost*>(port_textout_host()), Framed(game, 29, 188, data.loaded_text));
      else {
        opeu_ui_text(game, static_cast<TextHost*>(port_textout_host()), Framed(game, 29, 188, data.load_failed_text));
        choice = 0;
      }
      opeu_delay_units(game, static_cast<std::uint8_t>(15));
      opeu_text_paper_set(game, static_cast<TextHost*>(port_textout_host()), static_cast<std::uint8_t>(NORMAL));
    }
    opeu_dos_close(static_cast<std::int16_t>(file));
  }
  opeu_backdrop_fill_color(game, static_cast<UiHost const*>(port_uiprim_host()), static_cast<std::uint8_t>(1));
  return choice;
}
