// Redrawing a battle cell, and the strip above it with the cells to its left and above, through the VRAM save
// buffer (byte-aligned or odd columns).
#include "map/btlcell.hpp"
#include "common/battle-field.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "map/btldraw.hpp"
#include "map/btlsprt.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
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

[[nodiscard]] auto VramBuffer(Game& game) -> uint16_t {
  return Near(Data(game), Data(game).vram);
}

[[nodiscard]] auto OddColumn(uint16_t x) -> bool {
  return (x & 7) != 0;
}

// Saves the screen at the pixel position into the VRAM buffer.
auto SaveVram(Game& game, uint16_t x, uint16_t y) -> void {
  auto const buffer{ VramBuffer(game) };
  if (OddColumn(x))
    opeu_vram_save_odd(x >> 3, y, game.State().Bytes(buffer).data());
  else
    opeu_vram_save(x >> 3, y, game.State().Bytes(buffer).data());
}

// Restores the screen at the pixel position from the VRAM buffer.
auto RestoreVram(Game& game, uint16_t x, uint16_t y) -> void {
  auto const buffer{ VramBuffer(game) };
  if (OddColumn(x))
    opeu_vram_restore_odd(x >> 3, y, game.State().Bytes(buffer).data());
  else
    opeu_vram_restore(x >> 3, y, game.State().Bytes(buffer).data());
}

void DrawTerrain(Game& game, SpriteVariant variant, uint8_t x, uint8_t y) {
  auto& data{ Data(game) };
  data.sprite_variant[0] = Bits(variant);
  auto const terrain{ static_cast<uint8_t>(BattleTerrainAt(data, x, y) & BATTLE_TERRAIN_MASK) };
  if (opeu_battle_sprite_valid(game, terrain)) opeu_battle_draw_sprite(game, opeu_battle_sprite_offset(game, terrain));
}
} // namespace

void opeu_battle_draw_cell(Game& game, uint8_t x, uint8_t y) {
  auto&      data{ Data(game) };
  auto const column{ opeu_battle_cell_column(game, x, y) };
  auto const row{ opeu_battle_cell_row(game, x, y) };
  SaveVram(game, column, row);
  data.sprite_variant[0] = Bits(SpriteVariant::WHOLE);
  opeu_battle_draw_terrain(game, GridCell{ x, y }, BattleTerrainAt(data, x, y));
  opeu_battle_draw_neighbors(game, x, y);
  RestoreVram(game, column, row);
}

void opeu_battle_draw_cell_above(Game& game, uint8_t x, uint8_t y) {
  auto const column{ opeu_battle_cell_column(game, x, y) };
  auto const row{ static_cast<uint16_t>(opeu_battle_cell_row(game, x, y) - CELL_HEIGHT) };
  SaveVram(game, column, row);
  if (x != 0) DrawTerrain(game, SpriteVariant::LEFT, static_cast<uint8_t>(x - 1), y);
  if (y != 0) DrawTerrain(game, SpriteVariant::ABOVE, x, static_cast<uint8_t>(y - 1));
  if (x < BATTLE_WIDTH && y < BATTLE_WIDTH) DrawTerrain(game, SpriteVariant::CELL, x, y);
  RestoreVram(game, column, row);
}
