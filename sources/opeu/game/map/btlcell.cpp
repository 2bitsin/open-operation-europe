// Redrawing a battle cell, and the strip above it with the cells to its left and above, through the VRAM save
// buffer (byte-aligned or odd columns).
#include "map/btlcell.hpp"
#include "common/battle-field.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "map/btldraw.hpp"
#include "map/btlsprt.hpp"
#include "types.hpp"

#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
// sprite_variant[0]: which part of a terrain sprite the next drawing takes.
enum class SpriteVariant : uint8_t {
  CELL  = 0,
  LEFT  = 1, // of the cell to the left
  WHOLE = 2,
  ABOVE = 3, // of the cell above
};
constexpr uint16_t CELL_HEIGHT{ 26 };

// Saves the screen at the pixel position into the VRAM buffer, or restores it from there.
void Vram(uint8_t* ds, BattleEvent event, bool save, uint16_t x, uint16_t y) {
  auto const buffer{ Near(Data(ds), Data(ds).vram) };
  auto const routine{ (x & 7) ? (save ? BattleRoutine::VRAM_SAVE_ODD : BattleRoutine::VRAM_RESTORE_ODD)
                              : (save ? BattleRoutine::VRAM_SAVE : BattleRoutine::VRAM_RESTORE) };
  Call(ds, event, routine, x >> 3, y, buffer);
}

void DrawTerrain(uint8_t* ds, BattleEvent event, SpriteVariant variant, uint8_t x, uint8_t y) {
  auto& data{ Data(ds) };
  data.sprite_variant[0] = Bits(variant);
  auto const terrain{ static_cast<uint8_t>(BattleTerrainAt(data, x, y) & BATTLE_TERRAIN_MASK) };
  if (opeu_battle_sprite_valid(ds, event, terrain))
    opeu_battle_draw_sprite(ds, event, opeu_battle_sprite_offset(ds, event, terrain));
}
} // namespace

void opeu_battle_draw_cell(uint8_t* ds, BattleEvent event, uint8_t x, uint8_t y) {
  auto&      data{ Data(ds) };
  auto const column{ opeu_battle_cell_column(ds, event, x, y) };
  auto const row{ opeu_battle_cell_row(ds, event, x, y) };
  Vram(ds, event, true, column, row);
  data.sprite_variant[0] = Bits(SpriteVariant::WHOLE);
  opeu_battle_draw_terrain(ds, event, x, y, BattleTerrainAt(data, x, y));
  opeu_battle_draw_neighbors(ds, event, x, y);
  Vram(ds, event, false, column, row);
}

void opeu_battle_draw_cell_above(uint8_t* ds, BattleEvent event, uint8_t x, uint8_t y) {
  auto const column{ opeu_battle_cell_column(ds, event, x, y) };
  auto const row{ static_cast<uint16_t>(opeu_battle_cell_row(ds, event, x, y) - CELL_HEIGHT) };
  Vram(ds, event, true, column, row);
  if (x != 0) DrawTerrain(ds, event, SpriteVariant::LEFT, static_cast<uint8_t>(x - 1), y);
  if (y != 0) DrawTerrain(ds, event, SpriteVariant::ABOVE, x, static_cast<uint8_t>(y - 1));
  if (x < BATTLE_WIDTH && y < BATTLE_WIDTH) DrawTerrain(ds, event, SpriteVariant::CELL, x, y);
  Vram(ds, event, false, column, row);
}

uint16_t svc_btlcell(uint8_t* ds, uint32_t address, const uint16_t* args, unsigned int count) {
  return port_service(ds, address, args, count);
}

auto port_routine_opeu_battle_draw_cell(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_battle_draw_cell(ds, svc_btlcell, static_cast<uint8_t>(Arg(args, count, 0)),
                        static_cast<uint8_t>(Arg(args, count, 1)));
  return 0;
}
