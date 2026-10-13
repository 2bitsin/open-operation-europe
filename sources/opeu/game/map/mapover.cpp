// Strategic overview: a unit's glyph and the redraw of one overview cell. Pixel x counts 8-pixel byte columns.
//
// A unit is drawn as its pending order's glyph while the order is under way (recolored for its side), otherwise as
// its selected element's type glyph. A cell is redrawn as its terrain tile, an objective marker, the side marker,
// a bridge overlay, then a visible unit.
#include "map/mapover.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/order.hpp"
#include "common/scenario.hpp"
#include "core/memory.hpp"
#include "core/recindex.hpp"
#include "map/blast.hpp"
#include "map/cellflag.hpp"
#include "map/grid.hpp"
#include "map/mappixel.hpp"
#include "map/query.hpp"
#include "types.hpp"
#include "units/orders.hpp"
#include "units/selelem.hpp"

#include "common/game.hpp"
#include "port.hpp"
#include "services.hpp"

#include <array>

using namespace opeu::game;

namespace {
// Glyph tables in the sprite segment: 1 x 8 glyphs of 24 bytes and their 8-byte masks.
constexpr uint16_t ORDER_GLYPHS{ 0x188 };
constexpr uint16_t ORDER_MASKS{ 0x140 };
constexpr uint16_t TYPE_GLYPHS{ 0x50 };
constexpr uint16_t TYPE_MASKS{ 0x00 };
constexpr uint16_t MARK_MASK{ 0x40 }; // the side marker on a marked cell
constexpr uint16_t MARK_GLYPH{ 0x110 };
constexpr uint16_t BRIDGE_GLYPH{ 0x128 }; // drawn with its own mask
constexpr uint16_t GLYPH_BYTES{ 24 };
constexpr uint16_t MASK_BYTES{ 8 };
constexpr uint16_t TILE_BYTES{ 32 };  // an overview tile in the overview tile segment
constexpr uint8_t  BRIDGE_CODE{ 10 }; // opeu_map_code of a bridge cell
// Draw modes of the objective marker, by its side: in the Ardennes, and elsewhere.
constexpr std::array<uint16_t, 2> BULGE_MARKER{ 2, 4 };
constexpr std::array<uint16_t, 2> MARKER{ 0x702, 0x701 };

// Composes the 1 x 8 glyph at segment:offset through the streamed mask at a pixel position.
void Compose(Game& game, uint16_t x, uint16_t y, uint16_t offset, uint16_t segment) {
  auto const& data{ Data(game) };
  game.Machine().Display().ComposeSprite({ x, y, 1, 8 }, { .and_mask = FarBytes(game, static_cast<std::uint16_t>(data.stream_ptr[1]), static_cast<std::uint16_t>(data.stream_ptr[0])),
                                                           .or_image = FarBytes(game, segment, offset),
                                                           .buffer   = FarBytes(game, static_cast<std::uint16_t>(data.bitmap_buffer), 0),
                                                           .flags    = 0 });
}

// side_status of any side byte, 255 included, read where the original reads it.
[[nodiscard]] auto SideStatus(Game& game, uint8_t side) -> uint16_t {
  return Word(game, static_cast<uint16_t>(Near(Data(game), Data(game).side_status) + 2 * side));
}

} // namespace

// The glyph is read into a DS buffer the far read, the recolouring and the compose service work on.
void opeu_overview_unit_draw(Game& game, uint16_t unit, uint16_t x, uint16_t y) {
  auto&               data{ Data(game) };
  ServiceLocals const locals{ GLYPH_BYTES };
  auto const          buffer{ locals.At(0) };
  auto const          draw{ [&](uint16_t glyphs, uint16_t mask) {
    opeu_read_far_bytes(game, port_far(SPRITE_SEGMENT, 0), static_cast<std::uint16_t>(glyphs), static_cast<std::uint16_t>(buffer), GLYPH_BYTES);
    opeu_sprite_recolor(game, buffer, GLYPH_BYTES, opeu_unit_side(unit));
    SetStream(data, SPRITE_SEGMENT, mask);
    Compose(game, x, y, buffer, DATA_SEGMENT);
  } };
  bool       order_shown{ false };
  auto const side{ opeu_unit_side(unit) };
  uint8_t    destination_x;
  uint8_t    destination_y;
  if (opeu_unit_order_destination(game, unit, &destination_x, &destination_y) && (SideStatus(game, side) == SIDE_PLAYED || Record<Unit>(data, unit).flags & Bits(UnitFlag::SPOTTED))) {
    auto const& order{ Record<Unit>(data, unit).orders[0] };
    auto const  kind{ order.kind };
    order_shown = ShowsUntilStarted(kind) ? !order.progress : order.progress < 100;
    if (order_shown) {
      auto const glyph{ data.order_glyphs[kind] };
      draw(static_cast<uint16_t>(ORDER_GLYPHS + glyph * GLYPH_BYTES), static_cast<uint16_t>(ORDER_MASKS + glyph * MASK_BYTES));
      if (kind == 1 || kind == 2 || kind == 3) order_shown = false;
    }
  }
  if (order_shown) return;
  auto const& element{ Record<Element>(data, opeu_unit_selected_element(game, unit)) };
  auto const  glyph{ data.element_types[element.type].category };
  draw(static_cast<uint16_t>(TYPE_GLYPHS + glyph * GLYPH_BYTES), static_cast<uint16_t>(TYPE_MASKS + glyph * MASK_BYTES));
}

// Redraws the overview's cell (x, y) and the unit on it.
void opeu_overview_cell_draw(Game& game, uint8_t x, uint8_t y) {
  auto&      data{ Data(game) };
  auto const pixel{ OverviewPixel({ x, y }) };
  auto const pixel_x{ pixel.x };
  auto const pixel_y{ pixel.y };
  auto const tile{ data.map_tiles[MAP_WIDTH * y + x] };
  game.Machine().Display().DrawColumns({ static_cast<std::uint16_t>(pixel_x), static_cast<std::uint16_t>(pixel_y), 1, 8 },
                                       { 0, FarBytes(game, static_cast<std::uint16_t>(data.overview_tile_segment), static_cast<std::uint16_t>(tile * TILE_BYTES)) });
  if (auto const objective{ opeu_find_objective_at(game, x, y) }) {
    auto const& modes{ CurrentScenario(data) == Scenario::BULGE ? BULGE_MARKER : MARKER };
    auto const  mode{ modes[opeu_objective_side(game, objective) == 0 ? 0 : 1] };
    game.Machine().Display().DrawBitsOnPaper({ static_cast<std::uint16_t>(pixel_x), static_cast<std::uint16_t>(pixel_y), 1, 8 },
                                             { static_cast<std::uint16_t>(mode), game.State().Bytes(static_cast<std::uint16_t>(Near(data, data.overview_objective_glyph))) });
  }
  if (SideStatus(game, data.overview_side) == SIDE_PLAYED && opeu_cell_marked(game, x, y, data.overview_side)) {
    SetStream(data, SPRITE_SEGMENT, MARK_MASK);
    Compose(game, pixel_x, pixel_y, MARK_GLYPH, SPRITE_SEGMENT);
  }
  if (opeu_map_code(game, x, y) == BRIDGE_CODE) {
    SetStream(data, SPRITE_SEGMENT, BRIDGE_GLYPH);
    game.Machine().Display().DrawThreeByteColumns({ static_cast<std::uint16_t>(pixel_x), static_cast<std::uint16_t>(pixel_y), 1, 8 },
                                                  { 4, FarBytes(game, static_cast<std::uint16_t>(data.stream_ptr[1]), static_cast<std::uint16_t>(data.stream_ptr[0])) });
  }
  if (auto const unit{ opeu_find_unit_at(game, x, y) }; unit && opeu_unit_visible(game, unit)) opeu_overview_unit_draw(game, unit, pixel_x, pixel_y);
}
