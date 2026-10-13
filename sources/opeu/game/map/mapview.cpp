// Tactical view: a unit's sprite and the redraw of one map cell. Pixel x counts 8-pixel byte columns.
//
// A unit is drawn as its pending order's sprite while the order is under way (side 0 sprites recolored), otherwise as
// its selected element's type sprite, then its group/unit number. A cell is redrawn as its terrain tile, the side
// marker, an objective or bridge overlay, then a visible unit.
#include "map/mapview.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "common/order.hpp"
#include "core/memory.hpp"
#include "core/recindex.hpp"
#include "map/blast.hpp"
#include "map/cellflag.hpp"
#include "map/grid.hpp"
#include "map/mapcell.hpp"
#include "map/mapframe.hpp"
#include "map/mappixel.hpp"
#include "map/query.hpp"
#include "syslib-gfx.hpp"
#include "types.hpp"
#include "units/orders.hpp"
#include "units/selelem.hpp"
#include "vgalib.hpp"

using namespace opeu::game;

namespace {
// Sprite tables in the sprite segment. The sprites are copied to the DS stack and recolored for the unit's side; the
// masks go to the compositor as they are.
constexpr uint16_t ORDER_SPRITES{ 0x1838 };
constexpr uint16_t ORDER_MASKS{ 0x10a0 };
constexpr uint16_t TYPE_SPRITES{ 0x05f0 };
constexpr uint16_t TYPE_MASKS{ 0x0290 };
constexpr uint16_t SPRITE_BYTES{ 216 };   // a 3 x 24 sprite (and an order mask)
constexpr uint16_t TYPE_MASK_BYTES{ 72 }; // a type mask
constexpr uint16_t TILE_BYTES{ 0x120 };   // a terrain tile in the tile segment
constexpr uint8_t  OVERLAY_SPRITE{ 9 };   // the side marker on a marked cell
constexpr uint8_t  BRIDGE_SPRITE{ 11 };
constexpr uint8_t  BRIDGE_CODE{ 10 }; // opeu_map_code of a bridge cell
constexpr uint8_t  OBJECTIVE_CODE{ 1 };

// Copies a sprite from the sprite segment into the DS buffer `destination`.
void ReadSprite(Game& game, uint16_t offset, uint16_t destination) {
  opeu_read_far_bytes(game, port_far(SPRITE_SEGMENT, 0), offset, destination, SPRITE_BYTES);
}

// Composes the 3 x 24 sprite at sprite_segment:sprite_offset through the streamed mask at a pixel position.
void Compose(Game& game, uint16_t x, uint16_t y, uint16_t sprite_offset, uint16_t sprite_segment) {
  auto const& data{ Data(game) };
  opeu_sprite_compose(x, y, 3, 24, port_far(static_cast<std::uint16_t>(data.stream_ptr[1]), static_cast<std::uint16_t>(data.stream_ptr[0])), port_far(sprite_segment, sprite_offset),
                      port_far(static_cast<std::uint16_t>(data.bitmap_buffer), 0), 0);
}

} // namespace

// The sprite is read into a DS buffer the far read, the recolouring and the compose service work on.
void opeu_view_unit_draw(Game& game, uint16_t unit, uint16_t x, uint16_t y) {
  auto&               data{ Data(game) };
  ServiceLocals const locals{ SPRITE_BYTES };
  auto const          sprite_buffer{ locals.At(0) };
  auto const          side{ opeu_unit_side(unit) };
  auto const          recolor{ [&] { opeu_sprite_recolor(game, sprite_buffer, SPRITE_BYTES, side); } };
  bool                order_shown{ false };
  uint8_t             destination_x;
  uint8_t             destination_y;
  if (opeu_unit_order_destination(game, unit, &destination_x, &destination_y) && (data.side_status[side] == SIDE_PLAYED || Record<Unit>(data, unit).flags & Bits(UnitFlag::SPOTTED))) {
    auto const& order{ Record<Unit>(data, unit).orders[0] };
    auto const  kind{ order.kind };
    order_shown = ShowsUntilStarted(kind) ? !order.progress : order.progress < 100;
    if (order_shown) {
      auto const sprite{ data.order_glyphs[kind] };
      ReadSprite(game, static_cast<uint16_t>(ORDER_SPRITES + sprite * SPRITE_BYTES), sprite_buffer);
      recolor();
      SetStream(data, SPRITE_SEGMENT, static_cast<uint16_t>(ORDER_MASKS + sprite * SPRITE_BYTES));
      Compose(game, x, y, sprite_buffer, DATA_SEGMENT);
      if (kind == 1 || kind == 2 || kind == 3) order_shown = false;
    }
  }
  if (!order_shown) {
    auto const& element{ Record<Element>(data, opeu_unit_selected_element(game, unit)) };
    auto const  sprite{ data.element_types[element.type].category };
    ReadSprite(game, static_cast<uint16_t>(TYPE_SPRITES + sprite * SPRITE_BYTES), sprite_buffer);
    recolor();
    SetStream(data, SPRITE_SEGMENT, static_cast<uint16_t>(TYPE_MASKS + sprite * TYPE_MASK_BYTES));
    Compose(game, x, y, sprite_buffer, DATA_SEGMENT);
  }
  opeu_view_number(game, static_cast<uint16_t>(x * 8 + 18), static_cast<uint16_t>(y + 18), static_cast<uint16_t>(opeu_unit_group_index(unit) * 10 + opeu_unit_index(unit) + 11));
}

// Redraws the tactical view's cell (x, y) and the unit on it.
void opeu_view_cell_draw(Game& game, uint16_t x, uint16_t y) {
  auto&      data{ Data(game) };
  auto const cell_x{ static_cast<uint8_t>(x) };
  auto const cell_y{ static_cast<uint8_t>(y) };
  auto const pixel{ ViewPixel(data, { cell_x, cell_y }) };
  auto const pixel_x{ pixel.x };
  auto const pixel_y{ pixel.y };
  auto const tile{ data.map_tiles[MAP_WIDTH * cell_y + cell_x] };
  opeu_bitmap_restore(static_cast<std::uint16_t>(pixel_x), static_cast<std::uint16_t>(pixel_y), 3, 24, 0,
                      port_far(static_cast<std::uint16_t>(data.tile_segment), static_cast<std::uint16_t>(tile * TILE_BYTES)));
  auto const overlay{ [&](uint8_t sprite) {
    SetStream(data, SPRITE_SEGMENT, static_cast<uint16_t>(TYPE_MASKS + sprite * TYPE_MASK_BYTES));
    Compose(game, pixel_x, pixel_y, static_cast<uint16_t>(TYPE_SPRITES + sprite * SPRITE_BYTES), SPRITE_SEGMENT);
  } };
  if (data.side_status[data.view_side] == SIDE_PLAYED && opeu_cell_marked(game, cell_x, cell_y, data.view_side)) overlay(OVERLAY_SPRITE);
  switch (opeu_map_code(game, cell_x, cell_y)) {
  case BRIDGE_CODE:    overlay(BRIDGE_SPRITE); break;
  case OBJECTIVE_CODE: opeu_view_cell_paint(game, pixel_x, pixel_y, opeu_objective_side(game, opeu_find_objective_at(game, cell_x, cell_y)) ? 1 : 2); break;
  }
  auto const unit{ opeu_find_unit_at(game, cell_x, cell_y) };
  if (unit != 0 && opeu_unit_visible(game, unit)) opeu_view_unit_draw(game, unit, pixel_x, pixel_y);
}
