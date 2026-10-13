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

using namespace opeu::game;

namespace {

enum Service : std::uint16_t {
  EXISTS    = 0,  // (name) -> word
  OPEN      = 1,  // (name) -> handle
  SEEK      = 2,  // (handle, position low, position high, origin)
  READ      = 3,  // (handle, DS offset, count)
  CLOSE     = 4,  // (handle)
  PRINT     = 5,  // (format, value)
  ATTRIBUTE = 6,  // (paper)
  FRAMED    = 7,  // (x, y, text) -> DS offset of the framed text
  UI_TEXT   = 8,  // (text)
  WAIT      = 9,  // (units)
  MENU      = 10, // (menu, first, count) -> choice
  BACKDROP  = 11, // (mode)
};

constexpr std::uint16_t HIGHLIGHT{ 4 };
constexpr std::uint16_t NORMAL{ 0 };

auto Framed(Game& game, HostEvent event, std::uint16_t x, std::uint16_t y, auto const& text) -> std::uint16_t {
  return Call(game, event, FRAMED, x, y, Near(Data(game), text));
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
auto opeu_load_saved_game(Game& game, HostEvent event) -> std::uint16_t {
  auto&         data{ Data(game) };
  std::uint16_t choice{ 0 };
  Call(game, event, ATTRIBUTE, HIGHLIGHT);
  if (!Call(game, event, EXISTS, Near(data, data.savedata_name))) {
    Call(game, event, PRINT, Near(data, data.load_print_format), Framed(game, event, 32, 188, data.no_saved_games_text));
    Call(game, event, WAIT, 23);
    Call(game, event, ATTRIBUTE, NORMAL);
  } else {
    Call(game, event, ATTRIBUTE, NORMAL);
    auto const file{ Call(game, event, OPEN, Near(data, data.load_savedata_name)) };
    for (auto& slot : data.save_slots) Call(game, event, READ, file, Near(data, slot), sizeof slot);
    Call(game, event, ATTRIBUTE, HIGHLIGHT);
    Call(game, event, UI_TEXT, Framed(game, event, 33, 336, data.load_which_text));
    Call(game, event, ATTRIBUTE, NORMAL);
    if ((choice = Call(game, event, MENU, Near(data, data.save_list), 1, 3)) != 0) {
      Call(game, event, ATTRIBUTE, HIGHLIGHT);
      Call(game, event, UI_TEXT, Framed(game, event, 32, 188, data.loading_text));
      auto const position{ static_cast<std::uint32_t>(SAVED_BYTES * data.save_slot + SAVED_BLOCKS_AT) };
      Call(game, event, SEEK, file, static_cast<std::uint16_t>(position), static_cast<std::uint16_t>(position >> 16), SEEK_FROM_START);
      Call(game, event, READ, file, SAVED_FIRST, SAVED_BYTES);
      opeu_rebase_scenario(game, 1);
      auto const& slot{ data.save_slots[data.save_slot] };
      if (opeu_saved_game_valid(game, data.save_slot) && slot.scenario != SLOT_EMPTY)
        Call(game, event, UI_TEXT, Framed(game, event, 29, 188, data.loaded_text));
      else {
        Call(game, event, UI_TEXT, Framed(game, event, 29, 188, data.load_failed_text));
        choice = 0;
      }
      Call(game, event, WAIT, 15);
      Call(game, event, ATTRIBUTE, NORMAL);
    }
    Call(game, event, CLOSE, file);
  }
  Call(game, event, BACKDROP, 1);
  return choice;
}

auto svc_loadgame(Game& game, std::uint16_t id, std::uint16_t const* args, std::uint16_t count) -> std::uint16_t {
  switch (id) {
  case EXISTS: return static_cast<std::uint16_t>(opeu_dos_exists(reinterpret_cast<char const*>(game.State().Bytes(Arg(args, count, 0)).data())));
  case OPEN:   return static_cast<std::uint16_t>(opeu_open_resource(game, svc_resopen, Arg(args, count, 0)));
  case SEEK:
    return static_cast<std::uint16_t>(opeu_dos_seek(static_cast<std::int16_t>(Arg(args, count, 0)), static_cast<std::int32_t>(Arg32(args, count, 1)), static_cast<std::int16_t>(Arg(args, count, 3))));
  case READ: return static_cast<std::uint16_t>(opeu_dos_read(static_cast<std::int16_t>(Arg(args, count, 0)), game.State().Bytes(Arg(args, count, 1), Arg(args, count, 2)).data(), Arg(args, count, 2)));
  case CLOSE:     return static_cast<std::uint16_t>(opeu_dos_close(static_cast<std::int16_t>(Arg(args, count, 0))));
  case PRINT:     opeu_cursor_print(game, { args, count }); break;
  case ATTRIBUTE: opeu_text_paper_set(game, static_cast<TextHost*>(port_textout_host()), static_cast<std::uint8_t>(Arg(args, count, 0))); break;
  case FRAMED:    return opeu_title_box(game, static_cast<UiHost const*>(port_uiprim_host()), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2));
  case UI_TEXT:   opeu_ui_text(game, static_cast<TextHost*>(port_textout_host()), Arg(args, count, 0)); break;
  case WAIT:      opeu_delay_units(game, svc_waitkey, static_cast<std::uint8_t>(Arg(args, count, 0))); break;
  case MENU:      return opeu_list_dialog(game, svc_listdlg, Arg(args, count, 0), static_cast<std::uint8_t>(Arg(args, count, 1)), static_cast<std::uint8_t>(Arg(args, count, 2)));
  case BACKDROP:  opeu_backdrop_fill_color(game, static_cast<UiHost const*>(port_uiprim_host()), static_cast<std::uint8_t>(Arg(args, count, 0))); break;
  default:        port_unhandled("loadgame", id, count);
  }
  return 0;
}
