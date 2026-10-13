// The inspect highlight on a map cell: in the tactical view the cell frame's mask (tile 3) and image (tile 2), in the
// overview the 1 x 8 cursor marker.
#include "map/highlite.hpp"
#include "common/data-segment.hpp"
#include "common/map-frame.hpp"
#include "common/memory.hpp"
#include "map/blast.hpp"
#include "map/mappixel.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "port.hpp"
#include "vgalib.hpp"

using namespace opeu::game;

namespace {
constexpr uint16_t MASK_MODE{ 0x20f };
constexpr uint16_t IMAGE_MODE{ 0x307 };
constexpr uint16_t MARKER_MODE{ 7 };
} // namespace

void opeu_map_highlight_view(Game& game, uint8_t x, uint8_t y) {
  auto&      data{ Data(game) };
  auto const pixel{ ViewPixel(data, { x, y }) };
  auto       tile{ StreamFrameTile(data, 3) };
  game.Machine().Display().DrawBits({ static_cast<std::uint16_t>(pixel.x), static_cast<std::uint16_t>(pixel.y), 3, 24 },
                                    { MASK_MODE, FarBytes(game, static_cast<std::uint16_t>(tile.segment), static_cast<std::uint16_t>(tile.offset)) });
  tile = StreamFrameTile(data, 2);
  game.Machine().Display().DrawBits({ static_cast<std::uint16_t>(pixel.x), static_cast<std::uint16_t>(pixel.y), 3, 24 },
                                    { IMAGE_MODE, FarBytes(game, static_cast<std::uint16_t>(tile.segment), static_cast<std::uint16_t>(tile.offset)) });
}

void opeu_map_highlight_overview(Game& game, uint8_t x, uint8_t y) {
  auto const pixel{ OverviewPixel({ x, y }) };
  game.Machine().Display().DrawBits({ static_cast<std::uint16_t>(pixel.x), static_cast<std::uint16_t>(pixel.y), 1, 8 },
                                    { MARKER_MODE, game.State().Bytes(static_cast<std::uint16_t>(Near(Data(game), Data(game).flash_marker))) });
}
