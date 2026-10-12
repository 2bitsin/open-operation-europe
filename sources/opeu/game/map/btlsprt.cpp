// Battle sprites: the sprite offsets, the terrain sprites and the overlays of the neighbouring cells.
#include "types.hpp"
#include "map/btlsprt.hpp"
#include "common/battle-field.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "map/btldraw.hpp"
#include "types/events.hpp"

#include "port.hpp"
#include "glue.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
// A sprite record in the sprite segment: its image, then its mask at MASK_OFFSET.
constexpr uint16_t SPRITE_BYTES{ 650 };
constexpr uint16_t MASK_OFFSET{ 0x208 };
// The terrain sprites come in four variants (sprite_variant); the overlays after them in one.
constexpr uint8_t TERRAIN_SPRITES{ 17 };
constexpr uint8_t VARIANTS{ 4 };
constexpr uint8_t OVERLAYS_AT{ 51 };
// Overlay sprites, and the terrain codes that use them.
constexpr uint8_t OVERLAY_0{ 17 };
constexpr uint8_t OVERLAY_1{ 18 };
constexpr uint8_t OVERLAY_2{ 19 };
constexpr uint8_t OVERLAY_3{ 20 };
constexpr uint8_t BRIDGE_A{ 21 };
constexpr uint8_t BRIDGE_B{ 24 };
constexpr uint8_t BLANK_CODE{ 14 };
constexpr uint8_t BRIDGE_A_CODE{ 31 };
constexpr uint8_t BRIDGE_B_CODE{ 32 };
// Variants of the neighbours' overlays: below, to the right and diagonally.
constexpr uint8_t VARIANT_BELOW{ 1 };
constexpr uint8_t VARIANT_RIGHT{ 3 };
constexpr uint8_t VARIANT_DIAGONAL{ 0 };
constexpr uint8_t LAST_CELL{ BATTLE_WIDTH - 1 };
} // namespace

uint16_t opeu_battle_sprite_offset(uint8_t* ds, BattleEvent, uint8_t sprite) {
  auto const& data{ Data(ds) };
  if (sprite < TERRAIN_SPRITES)
    return static_cast<uint16_t>((sprite * VARIANTS + data.sprite_variant[0]) *
                                 SPRITE_BYTES);
  return static_cast<uint16_t>((sprite + OVERLAYS_AT) * SPRITE_BYTES);
}

void opeu_battle_draw_sprite(uint8_t* ds, BattleEvent event, uint16_t offset) {
  auto const& data{ Data(ds) };
  Call(ds, event, BattleRoutine::GFX_WAIT);
  Call(ds, event, BattleRoutine::VRAM_LOAD, Near(data, data.vram),
       data.sprite_segment, offset + MASK_OFFSET);
  Call(ds, event, BattleRoutine::VRAM_DRAW, Near(data, data.vram),
       data.sprite_segment, offset);
}

// A city cell also shows its share of the garrison.
void opeu_battle_draw_extra(uint8_t* ds, BattleEvent event, uint8_t x,
                            uint8_t y, uint8_t code) {
  auto& data{ Data(ds) };
  opeu_battle_draw_sprite(ds, event,
                          opeu_battle_sprite_offset(ds, event, code));
  if (code != BATTLE_CITY)
    return;
  for (unsigned i{ 0 };; ++i) {
    auto const& extra{ BattleExtraAt(data, i) };
    if (extra.x == x && extra.y == y) {
      opeu_battle_draw_number(ds, event, extra.count);
      return;
    }
    if (extra.x == BATTLE_UNPLACED)
      return;
  }
}

void opeu_battle_draw_terrain(uint8_t* ds, BattleEvent event, uint8_t x,
                              uint8_t y, uint8_t code) {
  auto const variant{ static_cast<uint8_t>(code >> 6) };
  auto const terrain{ static_cast<uint8_t>(code & BATTLE_TERRAIN_MASK) };
  auto const offset{ [&](uint8_t sprite) {
    return opeu_battle_sprite_offset(ds, event, sprite);
  } };
  // The overlay of the variant; the original leaves the offset unset for the variants it has no case for.
  auto const two_way{ [&] {
    switch (variant) {
      case 0:  return offset(OVERLAY_0);
      case 1:  return offset(OVERLAY_3);
      default: return uint16_t{ 0 };
    }
  } };
  if (terrain == BLANK_CODE)
    opeu_battle_draw_sprite(ds, event, offset(OVERLAY_0));
  if (terrain == BRIDGE_A_CODE) {
    opeu_battle_draw_sprite(ds, event, two_way());
    opeu_battle_draw_sprite(ds, event, offset(BRIDGE_A));
  }
  if (terrain == BRIDGE_B_CODE) {
    opeu_battle_draw_sprite(ds, event, two_way());
    opeu_battle_draw_sprite(ds, event, offset(BRIDGE_B));
  }
  if (terrain >= 21 && terrain <= 30)
    opeu_battle_draw_sprite(ds, event, two_way());
  if (terrain >= 33 && terrain <= 43) {
    constexpr uint8_t four_way[]{ OVERLAY_0, OVERLAY_1, OVERLAY_2, OVERLAY_3 };
    opeu_battle_draw_sprite(ds, event, offset(four_way[variant]));
  }
  opeu_battle_draw_extra(ds, event, x, y, terrain);
}

uint16_t opeu_battle_sprite_valid(uint8_t*, BattleEvent, uint8_t sprite) {
  return sprite < TERRAIN_SPRITES;
}

void opeu_battle_draw_neighbors(uint8_t* ds, BattleEvent event, uint8_t x,
                                uint8_t y) {
  auto&      data{ Data(ds) };
  auto const overlay{ [&](uint8_t variant, uint8_t at_x, uint8_t at_y) {
    data.sprite_variant[0] = variant;
    auto const sprite{ static_cast<uint8_t>(BattleTerrainAt(data, at_x, at_y) &
                                            BATTLE_TERRAIN_MASK) };
    if (opeu_battle_sprite_valid(ds, event, sprite))
      opeu_battle_draw_sprite(ds, event,
                              opeu_battle_sprite_offset(ds, event, sprite));
  } };
  if (y < LAST_CELL)
    overlay(VARIANT_BELOW, x, y + 1);
  if (x < LAST_CELL)
    overlay(VARIANT_RIGHT, x + 1, y);
  if (x < LAST_CELL && y < LAST_CELL)
    overlay(VARIANT_DIAGONAL, x + 1, y + 1);
}

uint16_t svc_btlsprt(uint8_t* ds, uint32_t address, const uint16_t* args,
                     unsigned int count) {
  return port_service(ds, address, args, count);
}
