// Start-up: allocating the game's memory blocks (falling back to EMS buffers), opening the data files, and the
// out-of-memory notice.
#include "core/segsetup.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "core/emsmap.hpp"
#include "core/textout.hpp"
#include "types.hpp"

#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include "core/settings.hpp"
#include "core/uiprim.hpp"
#include "io/resopen.hpp"
#include "io/sound.hpp"
#include <utility>

using namespace opeu::game;

namespace {

// The services the start-up routines call through their event; 0..8 are the EMS services of core/emsmap.
enum class SetupService : uint16_t {
  EMS_LAST         = 8,
  PRINT            = 9,  // print a text at the cursor
  WAIT_KEY         = 10, // the key pressed, 0 for none
  QUIT             = 11, // quit to the system
  ALLOCATE         = 12, // allocate paragraphs of memory, 0 when short
  OPEN_RESOURCE    = 13, // open a data file by name
  DOS_HANDLERS     = 14, // install the break and critical-error handlers
  VGA_SETUP        = 15,
  IDENTITY_PALETTE = 16,
  VGA_CLEAR        = 17,
  PALETTE_TITLE    = 18, // the title screen's palette
  SOUND_INITIALIZE = 19,
};

// Allocates a memory block and stores its segment; returns the segment (0 when memory is short).
auto Allocate(uint8_t* ds, HostEvent event, uint16_t& segment, uint16_t paragraphs) -> uint16_t {
  return segment = Call(ds, event, SetupService::ALLOCATE, paragraphs);
}

// Opens a data file and stores its handle.
template <typename Name>
void Open(uint8_t* ds, HostEvent event, uint16_t& handle, Name const& name) {
  handle = Call(ds, event, SetupService::OPEN_RESOURCE, Near(Data(ds), name));
}

} // namespace

// Prints the three-line out-of-memory notice, waits for a key and quits. `name`, the segment that could not be
// allocated, is not shown.
void opeu_memory_fail(uint8_t ds[65536], HostEvent event, uint16_t) {
  auto const& data{ Data(ds) };
  Call(ds, event, SetupService::PRINT, Near(data, data.memory_message));
  Call(ds, event, SetupService::PRINT, Near(data, data.memory_advice));
  Call(ds, event, SetupService::PRINT, Near(data, data.memory_exit));
  while (!Call(ds, event, SetupService::WAIT_KEY));
  Call(ds, event, SetupService::QUIT);
}

// Allocates the twelve memory blocks; the last four fall back to an EMS buffer when memory is short and an EMS
// manager is present. A block that cannot be had ends the game with the out-of-memory notice.
void opeu_alloc_segments(uint8_t ds[65536], HostEvent event) {
  auto&      data{ Data(ds) };
  auto const need{ [&](uint16_t& segment, uint16_t paragraphs, auto const& name) {
    if (Allocate(ds, event, segment, paragraphs) == 0) opeu_memory_fail(ds, event, Near(Data(ds), name));
  } };
  need(data.overview_tile_segment, 0xc1, data.setup_name_bmapseg);
  need(data.icon_segment, 0x867, data.setup_name_hunitseg);
  need(data.image_segment, 0xc81, data.setup_name_grpseg);
  need(data.numdlg_save_segment, 0x1b1, data.setup_name_ms_winseg);
  need(data.vseg_segment, 0xa, data.setup_name_vseg);
  need(data.hex_display_segment, 0x65, data.setup_name_hdispseg);
  need(data.face_segment, 0x4b0, data.setup_name_armsseg);
  need(data.portrait_segment, 0x4b0, data.setup_name_kaoseg);
  int16_t const ems = opeu_ems_detect(ds, event);
  if (Allocate(ds, event, data.frame_segment, 0x280) == 0 && (ems == 0 || opeu_ems_alloc_frame(ds, event) == 0)) opeu_memory_fail(ds, event, Near(data, data.setup_name_mdispseg));
  if (Allocate(ds, event, data.numdlg_image_segment, 0x5b1) == 0 && (ems == 0 || opeu_ems_alloc_numdlg(ds, event) == 0)) opeu_memory_fail(ds, event, Near(data, data.setup_name_ms_seg));
  if (Allocate(ds, event, data.sprite_segment, 0xf14) == 0 && (ems == 0 || opeu_ems_alloc_sprite(ds, event) == 0)) opeu_memory_fail(ds, event, Near(data, data.setup_name_hfontseg));
  if (Allocate(ds, event, data.tile_segment, 0x6c1) == 0 && (ems == 0 || opeu_ems_alloc_tile(ds, event) == 0)) opeu_memory_fail(ds, event, Near(data, data.setup_name_mmapseg));
}

// Opens the portrait, arms and event data files.
void opeu_open_data_files(uint8_t ds[65536], HostEvent event) {
  auto& data{ Data(ds) };
  Open(ds, event, data.portrait_handle, data.face_file_name);
  Open(ds, event, data.face_handle, data.arms_file_name);
  Open(ds, event, data.message_picture_handle, data.event_file_name);
}

// Prepares the machine: system handlers, the screen and title palette, sound, memory, and the data files.
void opeu_app_prepare(uint8_t ds[65536], HostEvent event) {
  Call(ds, event, SetupService::DOS_HANDLERS);
  Call(ds, event, SetupService::VGA_SETUP);
  Call(ds, event, SetupService::IDENTITY_PALETTE);
  Call(ds, event, SetupService::VGA_CLEAR);
  Call(ds, event, SetupService::PALETTE_TITLE);
  Call(ds, event, SetupService::SOUND_INITIALIZE);
  opeu_alloc_segments(ds, event);
  Data(ds).startup_mode++;
  opeu_open_data_files(ds, event);
}

uint16_t svc_segsetup(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  if (id <= std::to_underlying(SetupService::EMS_LAST)) return svc_emsmap(ds, id, args, count);
  switch (static_cast<SetupService>(id)) {
  case SetupService::PRINT:            opeu_cursor_print(ds, { args, 1 }); break;
  case SetupService::WAIT_KEY:         return opeu_turn_input_b();
  case SetupService::QUIT:             opeu_quit_to_dos(ds, svc_settings); break;
  case SetupService::ALLOCATE:         return opeu_dos_alloc(Arg(args, count, 0));
  case SetupService::OPEN_RESOURCE:    return static_cast<uint16_t>(opeu_open_resource(ds, svc_resopen, Arg(args, count, 0)));
  case SetupService::DOS_HANDLERS:     koei_install_dos_handlers(); break;
  case SetupService::VGA_SETUP:        koei_2fea(); break;
  case SetupService::IDENTITY_PALETTE: koei_vga_identity_palette(); break;
  case SetupService::VGA_CLEAR:        opeu_vga_clear(); break;
  case SetupService::PALETTE_TITLE:    opeu_palette_title_screen(ds, (const UiHost*)port_uiprim_host()); break;
  case SetupService::SOUND_INITIALIZE: opeu_sound_initialize(ds, svc_sound); break;
  default:                             port_unhandled("segsetup", id, count);
  }
  return 0;
}
