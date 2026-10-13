// Start-up: allocating the game's memory blocks, opening the data files, and the out-of-memory notice.
#include "core/segsetup.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "core/textout.hpp"
#include "types.hpp"

#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include "common/game.hpp"
#include "core/settings.hpp"
#include "core/uiprim.hpp"
#include "io/resopen.hpp"
#include "io/sound.hpp"
#include <utility>

using namespace opeu::game;

namespace {

// The services the start-up routines call through their event.
enum class SetupService : uint16_t {
  PRINT            = 0, // print a text at the cursor
  WAIT_KEY         = 1, // the key pressed, 0 for none
  QUIT             = 2, // quit to the system
  ALLOCATE         = 3, // allocate paragraphs of memory, 0 when short
  OPEN_RESOURCE    = 4, // open a data file by name
  DOS_HANDLERS     = 5, // install the break and critical-error handlers
  VGA_SETUP        = 6,
  IDENTITY_PALETTE = 7,
  VGA_CLEAR        = 8,
  PALETTE_TITLE    = 9, // the title screen's palette
  SOUND_INITIALIZE = 10,
};

// Allocates a memory block and stores its segment; returns the segment (0 when memory is short).
auto Allocate(Game& game, HostEvent event, uint16_t& segment, uint16_t paragraphs) -> uint16_t {
  return segment = Call(game, event, SetupService::ALLOCATE, paragraphs);
}

// Opens a data file and stores its handle.
template <typename Name>
void Open(Game& game, HostEvent event, uint16_t& handle, Name const& name) {
  handle = Call(game, event, SetupService::OPEN_RESOURCE, Near(Data(game), name));
}

} // namespace

// Prints the three-line out-of-memory notice, waits for a key and quits. `name`, the segment that could not be
// allocated, is not shown.
void opeu_memory_fail(Game& game, HostEvent event, uint16_t) {
  auto const& data{ Data(game) };
  Call(game, event, SetupService::PRINT, Near(data, data.memory_message));
  Call(game, event, SetupService::PRINT, Near(data, data.memory_advice));
  Call(game, event, SetupService::PRINT, Near(data, data.memory_exit));
  while (!Call(game, event, SetupService::WAIT_KEY));
  Call(game, event, SetupService::QUIT);
}

// Allocates the twelve memory blocks. A block that cannot be had ends the game with the out-of-memory notice.
void opeu_alloc_segments(Game& game, HostEvent event) {
  auto&      data{ Data(game) };
  auto const need{ [&](uint16_t& segment, uint16_t paragraphs, auto const& name) {
    if (Allocate(game, event, segment, paragraphs) == 0) opeu_memory_fail(game, event, Near(Data(game), name));
  } };
  need(data.overview_tile_segment, 0xc1, data.setup_name_bmapseg);
  need(data.icon_segment, 0x867, data.setup_name_hunitseg);
  need(data.image_segment, 0xc81, data.setup_name_grpseg);
  need(data.numdlg_save_segment, 0x1b1, data.setup_name_ms_winseg);
  need(data.vseg_segment, 0xa, data.setup_name_vseg);
  need(data.hex_display_segment, 0x65, data.setup_name_hdispseg);
  need(data.face_segment, 0x4b0, data.setup_name_armsseg);
  need(data.portrait_segment, 0x4b0, data.setup_name_kaoseg);
  need(data.frame_segment, 0x280, data.setup_name_mdispseg);
  need(data.numdlg_image_segment, 0x5b1, data.setup_name_ms_seg);
  need(data.sprite_segment, 0xf14, data.setup_name_hfontseg);
  need(data.tile_segment, 0x6c1, data.setup_name_mmapseg);
}

// Opens the portrait, arms and event data files.
void opeu_open_data_files(Game& game, HostEvent event) {
  auto& data{ Data(game) };
  Open(game, event, data.portrait_handle, data.face_file_name);
  Open(game, event, data.face_handle, data.arms_file_name);
  Open(game, event, data.message_picture_handle, data.event_file_name);
}

// Prepares the machine: system handlers, the screen and title palette, sound, memory, and the data files.
void opeu_app_prepare(Game& game, HostEvent event) {
  Call(game, event, SetupService::DOS_HANDLERS);
  Call(game, event, SetupService::VGA_SETUP);
  Call(game, event, SetupService::IDENTITY_PALETTE);
  Call(game, event, SetupService::VGA_CLEAR);
  Call(game, event, SetupService::PALETTE_TITLE);
  Call(game, event, SetupService::SOUND_INITIALIZE);
  opeu_alloc_segments(game, event);
  Data(game).startup_mode++;
  opeu_open_data_files(game, event);
}

uint16_t svc_segsetup(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  switch (static_cast<SetupService>(id)) {
  case SetupService::PRINT:            opeu_cursor_print(game, { args, 1 }); break;
  case SetupService::WAIT_KEY:         return opeu_turn_input_b();
  case SetupService::QUIT:             opeu_quit_to_dos(game, svc_settings); break;
  case SetupService::ALLOCATE:         return opeu_dos_alloc(Arg(args, count, 0));
  case SetupService::OPEN_RESOURCE:    return static_cast<uint16_t>(opeu_open_resource(game, Arg(args, count, 0)));
  case SetupService::DOS_HANDLERS:     koei_install_dos_handlers(); break;
  case SetupService::VGA_SETUP:        koei_2fea(); break;
  case SetupService::IDENTITY_PALETTE: koei_vga_identity_palette(); break;
  case SetupService::VGA_CLEAR:        opeu_vga_clear(); break;
  case SetupService::PALETTE_TITLE:    opeu_palette_title_screen(game, (const UiHost*)port_uiprim_host()); break;
  case SetupService::SOUND_INITIALIZE: opeu_sound_initialize(game, svc_sound); break;
  default:                             port_unhandled("segsetup", id, count);
  }
  return 0;
}
