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

#include "common/game.hpp"
#include "link/glue.hpp"
#include "link/link.hpp"
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
auto SaveVram(Game& game, BattleEvent event, uint16_t x, uint16_t y) -> void {
  auto const buffer{ VramBuffer(game) };
  if (OddColumn(x))
    Call(game, event, BattleRoutine::VRAM_SAVE_ODD, x >> 3, y, buffer);
  else
    Call(game, event, BattleRoutine::VRAM_SAVE, x >> 3, y, buffer);
}

// Restores the screen at the pixel position from the VRAM buffer.
auto RestoreVram(Game& game, BattleEvent event, uint16_t x, uint16_t y) -> void {
  auto const buffer{ VramBuffer(game) };
  if (OddColumn(x))
    Call(game, event, BattleRoutine::VRAM_RESTORE_ODD, x >> 3, y, buffer);
  else
    Call(game, event, BattleRoutine::VRAM_RESTORE, x >> 3, y, buffer);
}

void DrawTerrain(Game& game, BattleEvent event, SpriteVariant variant, uint8_t x, uint8_t y) {
  auto& data{ Data(game) };
  data.sprite_variant[0] = Bits(variant);
  auto const terrain{ static_cast<uint8_t>(BattleTerrainAt(data, x, y) & BATTLE_TERRAIN_MASK) };
  if (opeu_battle_sprite_valid(game, event, terrain)) opeu_battle_draw_sprite(game, event, opeu_battle_sprite_offset(game, event, terrain));
}
} // namespace

void opeu_battle_draw_cell(Game& game, BattleEvent event, uint8_t x, uint8_t y) {
  auto&      data{ Data(game) };
  auto const column{ opeu_battle_cell_column(game, event, x, y) };
  auto const row{ opeu_battle_cell_row(game, event, x, y) };
  SaveVram(game, event, column, row);
  data.sprite_variant[0] = Bits(SpriteVariant::WHOLE);
  opeu_battle_draw_terrain(game, event, x, y, BattleTerrainAt(data, x, y));
  opeu_battle_draw_neighbors(game, event, x, y);
  RestoreVram(game, event, column, row);
}

void opeu_battle_draw_cell_above(Game& game, BattleEvent event, uint8_t x, uint8_t y) {
  auto const column{ opeu_battle_cell_column(game, event, x, y) };
  auto const row{ static_cast<uint16_t>(opeu_battle_cell_row(game, event, x, y) - CELL_HEIGHT) };
  SaveVram(game, event, column, row);
  if (x != 0) DrawTerrain(game, event, SpriteVariant::LEFT, static_cast<uint8_t>(x - 1), y);
  if (y != 0) DrawTerrain(game, event, SpriteVariant::ABOVE, x, static_cast<uint8_t>(y - 1));
  if (x < BATTLE_WIDTH && y < BATTLE_WIDTH) DrawTerrain(game, event, SpriteVariant::CELL, x, y);
  RestoreVram(game, event, column, row);
}

uint16_t svc_btlcell(Game& game, uint32_t address, const uint16_t* args, unsigned int count) {
  return port_service(game, address, args, count);
}

auto port_routine_opeu_battle_draw_cell(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_battle_draw_cell(game, svc_btlcell, static_cast<uint8_t>(Arg(args, count, 0)), static_cast<uint8_t>(Arg(args, count, 1)));
  return 0;
}
