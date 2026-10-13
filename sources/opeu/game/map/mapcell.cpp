// Painting one tactical-view cell frame, and flashing the cursor on a map cell in whichever map is up.
#include "map/mapcell.hpp"
#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/map-frame.hpp"
#include "common/memory.hpp"
#include "map/blast.hpp"
#include "map/mappixel.hpp"
#include "types.hpp"
#include "vgalib.hpp"

using namespace opeu::game;

namespace {
constexpr uint16_t BORDER_MODE{ 0x20f };
constexpr uint16_t IMAGE_MODE{ 0x300 }; // added to the caller's mode
constexpr uint16_t HIGHLIGHT_MODE{ 7 };
} // namespace

// The cell frame's border mask (tile 1), then its image (tile 0) in the given mode, both 3 x 24 at (x, y).
void opeu_view_cell_paint(Game& game, uint16_t x, uint16_t y, uint16_t mode) {
  auto& data{ Data(game) };
  auto  tile{ StreamFrameTile(data, 1) };
  game.Machine().Display().DrawBits({ x, y, 3, 24 }, { BORDER_MODE, FarBytes(game, static_cast<std::uint16_t>(tile.segment), static_cast<std::uint16_t>(tile.offset)) });
  tile = StreamFrameTile(data, 0);
  game.Machine().Display().DrawBits({ x, y, 3, 24 },
                                    { static_cast<std::uint16_t>(mode | IMAGE_MODE), FarBytes(game, static_cast<std::uint16_t>(tile.segment), static_cast<std::uint16_t>(tile.offset)) });
}

// Overview: alternately draws the cursor marker over the saved 1 x 8 background and restores the background. Tactical
// view: repaints the cell highlighted.
void opeu_map_cell_flash(Game& game, uint8_t x, uint8_t y) {
  auto&      data{ Data(game) };
  auto const buffer{ Near(data, data.flash_buffers) };
  if (data.overview_side != NO_SIDE) {
    auto const pixel{ OverviewPixel({ x, y }) };
    if (data.flash_state) {
      game.Machine().Display().DrawColumns({ static_cast<std::uint16_t>(pixel.x), static_cast<std::uint16_t>(pixel.y), 1, 8 }, { 0, game.State().Bytes(static_cast<std::uint16_t>(buffer)) });
      data.flash_state = 0;
    } else {
      game.Machine().Display().SaveColumns({ static_cast<std::uint16_t>(pixel.x), static_cast<std::uint16_t>(pixel.y), 1, 8 }, game.State().Bytes(static_cast<std::uint16_t>(buffer)));
      game.Machine().Display().DrawBits({ static_cast<std::uint16_t>(pixel.x), static_cast<std::uint16_t>(pixel.y), 1, 8 },
                                        { HIGHLIGHT_MODE, game.State().Bytes(static_cast<std::uint16_t>(Near(data, data.flash_marker))) });
      data.flash_state = 1;
    }
  } else if (data.view_side != NO_SIDE) {
    auto const pixel{ ViewPixel(data, { x, y }) };
    opeu_view_cell_paint(game, pixel.x, pixel.y, HIGHLIGHT_MODE);
  }
}
