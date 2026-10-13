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
#include <array>
#include <utility>

using namespace opeu::game;

namespace {

// Allocates a memory block and stores its segment; returns the segment (0 when memory is short).
auto Allocate(Game& game, uint16_t& segment, uint16_t paragraphs) -> uint16_t {
  return segment = opeu_dos_alloc(paragraphs);
}

// Opens a data file and stores its handle.
template <typename Name>
void Open(Game& game, uint16_t& handle, Name const& name) {
  handle = static_cast<uint16_t>(opeu_open_resource(game, Near(Data(game), name)));
}

} // namespace

// Prints the three-line out-of-memory notice, waits for a key and quits. `name`, the segment that could not be
// allocated, is not shown.
void opeu_memory_fail(Game& game, uint16_t) {
  auto const& data{ Data(game) };
  opeu_cursor_print(game, std::array<std::uint16_t, 1>{ Near(data, data.memory_message) });
  opeu_cursor_print(game, std::array<std::uint16_t, 1>{ Near(data, data.memory_advice) });
  opeu_cursor_print(game, std::array<std::uint16_t, 1>{ Near(data, data.memory_exit) });
  while (!opeu_turn_input_b());
  opeu_quit_to_dos(game);
}

// Allocates the twelve memory blocks. A block that cannot be had ends the game with the out-of-memory notice.
void opeu_alloc_segments(Game& game) {
  auto&      data{ Data(game) };
  auto const need{ [&](uint16_t& segment, uint16_t paragraphs, auto const& name) {
    if (Allocate(game, segment, paragraphs) == 0) opeu_memory_fail(game, Near(Data(game), name));
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
void opeu_open_data_files(Game& game) {
  auto& data{ Data(game) };
  Open(game, data.portrait_handle, data.face_file_name);
  Open(game, data.face_handle, data.arms_file_name);
  Open(game, data.message_picture_handle, data.event_file_name);
}

// Prepares the machine: system handlers, the screen and title palette, sound, memory, and the data files.
void opeu_app_prepare(Game& game) {
  koei_install_dos_handlers();
  koei_2fea();
  koei_vga_identity_palette();
  opeu_vga_clear();
  opeu_palette_title_screen(game, static_cast<const UiHost*>(port_uiprim_host()));
  opeu_sound_initialize(game);
  opeu_alloc_segments(game);
  Data(game).startup_mode++;
  opeu_open_data_files(game);
}
