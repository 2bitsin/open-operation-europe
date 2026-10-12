// Tactical view: a unit's sprite and the redraw of one map cell. Pixel x counts 8-pixel byte columns.
//
// A unit is drawn as its pending order's sprite while the order is under way (side 0 sprites recolored), otherwise as
// its selected element's type sprite, then its group/unit number. A cell is redrawn as its terrain tile, the side
// marker, an objective or bridge overlay, then a visible unit.
#include "types.hpp"
#include "map/mapview.hpp"
#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "core/recindex.hpp"
#include "map/blast.hpp"
#include "map/cellflag.hpp"
#include "map/grid.hpp"
#include "map/mapcell.hpp"
#include "map/mapframe.hpp"
#include "map/mappixel.hpp"
#include "map/query.hpp"
#include "units/orders.hpp"
#include "units/selelem.hpp"
#include "types/events.hpp"

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
void ReadSprite(uint8_t* ds, HostEvent event, uint16_t offset,
                uint16_t destination) {
  Call(ds, event, MapService::READ_FAR_BYTES, offset, SPRITE_SEGMENT,
       destination, SPRITE_BYTES);
}

// Points the image stream at a mask in the sprite segment.
void SetStream(DataSegment& data, uint16_t offset) {
  data.stream_ptr[0] = offset;
  data.stream_ptr[1] = SPRITE_SEGMENT;
}

// Composes the 3 x 24 sprite at sprite_segment:sprite_offset through the streamed mask at a pixel position.
void Compose(uint8_t* ds, HostEvent event, uint16_t x, uint16_t y,
             uint16_t sprite_offset, uint16_t sprite_segment) {
  auto const& data{ Data(ds) };
  Call(ds, event, MapService::SPRITE_COMPOSE, x, y, 3, 24, data.stream_ptr[0],
       data.stream_ptr[1], sprite_offset, sprite_segment, 0, data.bitmap_buffer,
       0);
}

// The order kinds whose sprite shows until the order has started; the others show until it is complete.
[[nodiscard]] auto ShowsUntilStarted(uint8_t kind) -> bool {
  return kind == 1 || kind == 2 || kind == 3 || kind == 13;
}
} // namespace

// frame: the routine's frame on the DS stack; its sprite buffer lives there.
void opeu_view_unit_draw(uint8_t* ds, HostEvent event, uint16_t frame,
                         uint16_t unit, uint16_t x, uint16_t y) {
  auto&      data{ Data(ds) };
  auto const sprite_buffer{ Local(frame, 0xe8) };
  auto const side{ opeu_unit_side(unit) };
  auto const recolor{ [&] {
    opeu_sprite_recolor(ds, sprite_buffer, SPRITE_BYTES, side);
  } };
  bool    order_shown{ false };
  uint8_t destination_x;
  uint8_t destination_y;
  if (opeu_unit_order_destination(ds, unit, &destination_x, &destination_y) &&
      (data.side_status[side] == SIDE_PLAYED ||
       Record<Unit>(data, unit).flags & Bits(UnitFlag::SPOTTED))) {
    auto const& order{ Record<Unit>(data, unit).orders[0] };
    auto const  kind{ order.kind };
    order_shown =
        ShowsUntilStarted(kind) ? !order.progress : order.progress < 100;
    if (order_shown) {
      auto const sprite{ data.order_glyphs[kind] };
      ReadSprite(ds, event,
                 static_cast<uint16_t>(ORDER_SPRITES + sprite * SPRITE_BYTES),
                 sprite_buffer);
      recolor();
      SetStream(data,
                static_cast<uint16_t>(ORDER_MASKS + sprite * SPRITE_BYTES));
      Compose(ds, event, x, y, sprite_buffer, DATA_SEGMENT);
      if (kind == 1 || kind == 2 || kind == 3)
        order_shown = false;
    }
  }
  if (!order_shown) {
    auto const& element{ Record<Element>(
        data, opeu_unit_selected_element(ds, unit)) };
    auto const  sprite{ data.element_types[element.type].category };
    ReadSprite(ds, event,
               static_cast<uint16_t>(TYPE_SPRITES + sprite * SPRITE_BYTES),
               sprite_buffer);
    recolor();
    SetStream(data,
              static_cast<uint16_t>(TYPE_MASKS + sprite * TYPE_MASK_BYTES));
    Compose(ds, event, x, y, sprite_buffer, DATA_SEGMENT);
  }
  opeu_view_number(ds, event, static_cast<uint16_t>(x * 8 + 18),
                   static_cast<uint16_t>(y + 18),
                   static_cast<uint16_t>(opeu_unit_group_index(unit) * 10 +
                                         opeu_unit_index(unit) + 11));
}

// frame: the routine's frame on the DS stack; the pixel position is converted in place there, and the unit drawing
// runs on the frame below.
void opeu_view_cell_draw(uint8_t* ds, HostEvent event, uint16_t frame,
                         uint16_t x, uint16_t y) {
  auto&      data{ Data(ds) };
  auto const pixel_x_at{ Local(frame, 2) };
  auto const pixel_y_at{ Local(frame, 6) };
  auto const cell_x{ static_cast<uint8_t>(x) };
  auto const cell_y{ static_cast<uint8_t>(y) };
  SetWord(ds, pixel_x_at, cell_x);
  SetWord(ds, pixel_y_at, cell_y);
  opeu_view_pixel(ds, pixel_x_at, pixel_y_at);
  auto const pixel_x{ Word(ds, pixel_x_at) };
  auto const pixel_y{ Word(ds, pixel_y_at) };
  Call(ds, event, MapService::VIEW_PREPARE);
  auto const tile{ data.map_tiles[MAP_WIDTH * cell_y + cell_x] };
  Call(ds, event, MapService::RESTORE_FAR, pixel_x, pixel_y, 3, 24, 0,
       data.tile_segment, tile * TILE_BYTES);
  auto const overlay{ [&](uint8_t sprite) {
    SetStream(data,
              static_cast<uint16_t>(TYPE_MASKS + sprite * TYPE_MASK_BYTES));
    Compose(ds, event, pixel_x, pixel_y,
            static_cast<uint16_t>(TYPE_SPRITES + sprite * SPRITE_BYTES),
            SPRITE_SEGMENT);
  } };
  if (data.side_status[data.view_side] == SIDE_PLAYED &&
      opeu_cell_marked(ds, cell_x, cell_y, data.view_side))
    overlay(OVERLAY_SPRITE);
  switch (opeu_map_code(ds, cell_x, cell_y)) {
    case BRIDGE_CODE: overlay(BRIDGE_SPRITE); break;
    case OBJECTIVE_CODE:
      opeu_view_cell_paint(
          ds, event, pixel_x, pixel_y,
          opeu_objective_side(ds, opeu_find_objective_at(ds, cell_x, cell_y))
              ? 1
              : 2);
      break;
  }
  auto const unit{ opeu_find_unit_at(ds, cell_x, cell_y) };
  if (unit != 0 && opeu_unit_visible(ds, unit))
    opeu_view_unit_draw(ds, event, Local(frame, 0x1c), unit, pixel_x, pixel_y);
}

// The services this module raises (restore, compose, view prepare, far read) are the map segment's shared ones.
uint16_t svc_mapview(uint8_t* ds, uint16_t kind, const uint16_t* args,
                     uint16_t count) {
  return svc_map_plan(ds, "mapview", kind, args, count);
}
