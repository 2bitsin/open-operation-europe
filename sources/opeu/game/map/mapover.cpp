// Strategic overview: a unit's glyph and the redraw of one overview cell. Pixel x counts 8-pixel byte columns.
//
// A unit is drawn as its pending order's glyph while the order is under way (recolored for its side), otherwise as
// its selected element's type glyph. A cell is redrawn as its terrain tile, an objective marker, the side marker,
// a bridge overlay, then a visible unit.
#include "map/mapover.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/scenario.hpp"
#include "core/recindex.hpp"
#include "map/blast.hpp"
#include "map/cellflag.hpp"
#include "map/grid.hpp"
#include "map/mappixel.hpp"
#include "map/query.hpp"
#include "types.hpp"
#include "units/orders.hpp"
#include "units/selelem.hpp"

#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

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
constexpr uint16_t BULGE_MARKER[2]{ 2, 4 };
constexpr uint16_t MARKER[2]{ 0x702, 0x701 };

// Points the image stream at sprite-segment data.
void SetStream(DataSegment& data, uint16_t offset) {
  data.stream_ptr[0] = offset;
  data.stream_ptr[1] = SPRITE_SEGMENT;
}

// Composes the 1 x 8 glyph at segment:offset through the streamed mask at a pixel position.
void Compose(uint8_t* ds, HostEvent event, uint16_t x, uint16_t y, uint16_t offset, uint16_t segment) {
  auto const& data{ Data(ds) };
  Call(ds, event, MapService::SPRITE_COMPOSE, x, y, 1, 8, data.stream_ptr[0], data.stream_ptr[1], offset, segment, 0, data.bitmap_buffer, 0);
}

// side_status of any side byte, 255 included, read where the original reads it.
[[nodiscard]] auto SideStatus(uint8_t* ds, uint8_t side) -> uint16_t {
  return Word(ds, static_cast<uint16_t>(Near(Data(ds), Data(ds).side_status) + 2 * side));
}

[[nodiscard]] auto ShowsUntilStarted(uint8_t kind) -> bool {
  return kind == 1 || kind == 2 || kind == 3 || kind == 13;
}
} // namespace

// The glyph is read into a DS buffer the far read, the recolouring and the compose service work on.
void opeu_overview_unit_draw(uint8_t* ds, HostEvent event, uint16_t unit, uint16_t x, uint16_t y) {
  auto&               data{ Data(ds) };
  ServiceLocals const locals{ GLYPH_BYTES };
  auto const          buffer{ locals.At(0) };
  auto const          draw{ [&](uint16_t glyphs, uint16_t mask) {
    Call(ds, event, MapService::READ_FAR_BYTES, glyphs, SPRITE_SEGMENT, buffer, GLYPH_BYTES);
    opeu_sprite_recolor(ds, buffer, GLYPH_BYTES, opeu_unit_side(unit));
    SetStream(data, mask);
    Compose(ds, event, x, y, buffer, DATA_SEGMENT);
  } };
  bool       order_shown{ false };
  auto const side{ opeu_unit_side(unit) };
  uint8_t    destination_x;
  uint8_t    destination_y;
  if (opeu_unit_order_destination(ds, unit, &destination_x, &destination_y) && (SideStatus(ds, side) == SIDE_PLAYED || Record<Unit>(data, unit).flags & Bits(UnitFlag::SPOTTED))) {
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
  auto const& element{ Record<Element>(data, opeu_unit_selected_element(ds, unit)) };
  auto const  glyph{ data.element_types[element.type].category };
  draw(static_cast<uint16_t>(TYPE_GLYPHS + glyph * GLYPH_BYTES), static_cast<uint16_t>(TYPE_MASKS + glyph * MASK_BYTES));
}

// Redraws the overview's cell (x, y) and the unit on it.
void opeu_overview_cell_draw(uint8_t* ds, HostEvent event, uint8_t x, uint8_t y) {
  auto&      data{ Data(ds) };
  auto const pixel{ OverviewPixel({ x, y }) };
  auto const pixel_x{ pixel.x };
  auto const pixel_y{ pixel.y };
  auto const tile{ data.map_tiles[MAP_WIDTH * y + x] };
  Call(ds, event, MapService::RESTORE_FAR, pixel_x, pixel_y, 1, 8, 0, data.overview_tile_segment, tile * TILE_BYTES);
  if (auto const objective{ opeu_find_objective_at(ds, x, y) }) {
    auto const& modes{ CurrentScenario(data) == Scenario::BULGE ? BULGE_MARKER : MARKER };
    auto const  mode{ modes[opeu_objective_side(ds, objective) == 0 ? 0 : 1] };
    Call(ds, event, MapService::DRAW_MASKED_NEAR, pixel_x, pixel_y, 1, 8, mode, Near(data, data.overview_objective_glyph));
  }
  if (SideStatus(ds, data.overview_side) == SIDE_PLAYED && opeu_cell_marked(ds, x, y, data.overview_side)) {
    SetStream(data, MARK_MASK);
    Compose(ds, event, pixel_x, pixel_y, MARK_GLYPH, SPRITE_SEGMENT);
  }
  if (opeu_map_code(ds, x, y) == BRIDGE_CODE) {
    SetStream(data, BRIDGE_GLYPH);
    Call(ds, event, MapService::BLIT_FAR, pixel_x, pixel_y, 1, 8, 4, data.stream_ptr[1], data.stream_ptr[0]);
  }
  if (auto const unit{ opeu_find_unit_at(ds, x, y) }; unit && opeu_unit_visible(ds, unit)) opeu_overview_unit_draw(ds, event, unit, pixel_x, pixel_y);
}

// Services of this module: the 15c4 segment's shared map services (map/blast).
uint16_t svc_mapover(uint8_t* ds, uint16_t kind, const uint16_t* args, uint16_t count) {
  return svc_map_plan(ds, "mapover", kind, args, count);
}
