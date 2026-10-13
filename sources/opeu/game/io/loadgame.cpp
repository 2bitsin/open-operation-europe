// The load menu and the saved-game checksum.
#include "io/loadgame.hpp"

#include "common/data-segment.hpp"
#include "common/dos-file.hpp"
#include "common/memory.hpp"
#include "common/saved-game.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "glue.hpp"
#include "io/listdlg.hpp"
#include "io/rebase.hpp"
#include "io/resopen.hpp"
#include "io/waitkey.hpp"
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

auto Framed(std::uint8_t* ds, HostEvent event, std::uint16_t x, std::uint16_t y, auto const& text) -> std::uint16_t {
  return Call(ds, event, FRAMED, x, y, Near(Data(ds), text));
}
} // namespace

// 1 when the byte sum of the saved part of the data segment equals the slot's checksum.
auto opeu_saved_game_valid(std::uint8_t* ds, std::uint8_t slot) -> std::uint16_t {
  auto const& data{ Data(ds) };
  return data.save_slots[slot].checksum == SavedChecksum(data) ? 1 : 0;
}

auto opeu_save_record_size() -> std::uint16_t {
  return SAVED_BYTES;
}

// The load menu: reads the 20 slot headers from SAVEDATA, lets the player choose a slot, loads and rebases that
// game and reports whether it is valid. Returns the menu choice, or 0 when nothing valid was loaded.
auto opeu_load_saved_game(std::uint8_t* ds, HostEvent event) -> std::uint16_t {
  auto&         data{ Data(ds) };
  std::uint16_t choice{ 0 };
  Call(ds, event, ATTRIBUTE, HIGHLIGHT);
  if (!Call(ds, event, EXISTS, Near(data, data.savedata_name))) {
    Call(ds, event, PRINT, Near(data, data.load_print_format), Framed(ds, event, 32, 188, data.no_saved_games_text));
    Call(ds, event, WAIT, 23);
    Call(ds, event, ATTRIBUTE, NORMAL);
  } else {
    Call(ds, event, ATTRIBUTE, NORMAL);
    auto const file{ Call(ds, event, OPEN, Near(data, data.load_savedata_name)) };
    for (auto& slot : data.save_slots) Call(ds, event, READ, file, Near(data, slot), sizeof slot);
    Call(ds, event, ATTRIBUTE, HIGHLIGHT);
    Call(ds, event, UI_TEXT, Framed(ds, event, 33, 336, data.load_which_text));
    Call(ds, event, ATTRIBUTE, NORMAL);
    if ((choice = Call(ds, event, MENU, Near(data, data.save_list), 1, 3)) != 0) {
      Call(ds, event, ATTRIBUTE, HIGHLIGHT);
      Call(ds, event, UI_TEXT, Framed(ds, event, 32, 188, data.loading_text));
      auto const position{ static_cast<std::uint32_t>(SAVED_BYTES * data.save_slot + SAVED_BLOCKS_AT) };
      Call(ds, event, SEEK, file, static_cast<std::uint16_t>(position), static_cast<std::uint16_t>(position >> 16), SEEK_FROM_START);
      Call(ds, event, READ, file, SAVED_FIRST, SAVED_BYTES);
      opeu_rebase_scenario(ds, 1);
      auto const& slot{ data.save_slots[data.save_slot] };
      if (opeu_saved_game_valid(ds, data.save_slot) && slot.scenario != SLOT_EMPTY)
        Call(ds, event, UI_TEXT, Framed(ds, event, 29, 188, data.loaded_text));
      else {
        Call(ds, event, UI_TEXT, Framed(ds, event, 29, 188, data.load_failed_text));
        choice = 0;
      }
      Call(ds, event, WAIT, 15);
      Call(ds, event, ATTRIBUTE, NORMAL);
    }
    Call(ds, event, CLOSE, file);
  }
  Call(ds, event, BACKDROP, 1);
  return choice;
}

auto svc_loadgame(std::uint8_t* ds, std::uint16_t id, std::uint16_t const* args, std::uint16_t count) -> std::uint16_t {
  switch (id) {
  case EXISTS: return static_cast<std::uint16_t>(opeu_dos_exists(reinterpret_cast<char const*>(PORT_DS(Arg(args, count, 0)))));
  case OPEN:   return static_cast<std::uint16_t>(opeu_open_resource(ds, svc_resopen, Arg(args, count, 0)));
  case SEEK:
    return static_cast<std::uint16_t>(opeu_dos_seek(static_cast<std::int16_t>(Arg(args, count, 0)), static_cast<std::int32_t>(Arg32(args, count, 1)), static_cast<std::int16_t>(Arg(args, count, 3))));
  case READ:      return static_cast<std::uint16_t>(opeu_dos_read(static_cast<std::int16_t>(Arg(args, count, 0)), PORT_DS(Arg(args, count, 1)), Arg(args, count, 2)));
  case CLOSE:     return static_cast<std::uint16_t>(opeu_dos_close(static_cast<std::int16_t>(Arg(args, count, 0))));
  case PRINT:     opeu_cursor_print(ds, { args, count }); break;
  case ATTRIBUTE: opeu_text_paper_set(ds, static_cast<TextHost*>(port_textout_host()), static_cast<std::uint8_t>(Arg(args, count, 0))); break;
  case FRAMED:    return opeu_title_box(ds, static_cast<UiHost const*>(port_uiprim_host()), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2));
  case UI_TEXT:   opeu_ui_text(ds, static_cast<TextHost*>(port_textout_host()), Arg(args, count, 0)); break;
  case WAIT:      opeu_delay_units(ds, svc_waitkey, static_cast<std::uint8_t>(Arg(args, count, 0))); break;
  case MENU:      return opeu_list_dialog(ds, svc_listdlg, Arg(args, count, 0), static_cast<std::uint8_t>(Arg(args, count, 1)), static_cast<std::uint8_t>(Arg(args, count, 2)));
  case BACKDROP:  opeu_backdrop_fill_color(ds, static_cast<UiHost const*>(port_uiprim_host()), static_cast<std::uint8_t>(Arg(args, count, 0))); break;
  default:        port_unhandled("loadgame", id, count);
  }
  return 0;
}
