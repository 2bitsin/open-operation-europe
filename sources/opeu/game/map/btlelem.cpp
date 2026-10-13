// Battle elements on the map: placing and drawing one on its cell, redrawing a cell, and animating an element.
#include "map/btlelem.hpp"
#include "common/battle-field.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "io/sound.hpp"
#include "map/btlcalc.hpp"
#include "map/btlcell.hpp"
#include "map/btldraw.hpp"
#include "map/btlicon.hpp"
#include "map/btllist.hpp"
#include "map/btlsprt.hpp"
#include "map/btlunit.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <ranges>

using namespace opeu::game;

namespace {
// Sprite_variant while a cell's terrain is drawn under an element.
constexpr uint8_t  TERRAIN_VARIANT{ 2 };
constexpr uint16_t ICON_BYTES{ 520 };
// An animation frame: a bitmap and its mask.
constexpr uint16_t ICON_STRIDE{ 650 };
constexpr uint8_t  ANIMATION_FRAMES{ 4 };
constexpr uint8_t  ANIMATION_HANDLER{ 5 };

// Saves (and restores) the screen under a cell; a column not on a byte boundary takes the odd routine.
void SaveCell(Game& game, uint16_t column, uint16_t row) {
  (column & 7) ? opeu_vram_save_odd(column >> 3, row, game.State().Bytes(Near(Data(game), Data(game).vram)).data())
               : opeu_vram_save(column >> 3, row, game.State().Bytes(Near(Data(game), Data(game).vram)).data());
}
void RestoreCell(Game& game, uint16_t column, uint16_t row) {
  (column & 7) ? opeu_vram_restore_odd(column >> 3, row, game.State().Bytes(Near(Data(game), Data(game).vram)).data())
               : opeu_vram_restore(column >> 3, row, game.State().Bytes(Near(Data(game), Data(game).vram)).data());
}

// One frame of an animation over the cell, with the element under it.
void DrawFrame(Game& game, GridCell cell, CellAnimation const& animation, uint8_t frame) {
  auto&      data{ Data(game) };
  auto const column{ opeu_battle_cell_column(game, cell.x, cell.y) };
  auto const row{ opeu_battle_cell_row(game, cell.x, cell.y) };
  SaveCell(game, column, row);
  data.sprite_variant[0] = TERRAIN_VARIANT;
  auto const terrain{ BattleTerrainAt(data, cell.x, cell.y) };
  opeu_battle_draw_terrain(game, cell, terrain);
  if (auto const element{ animation.element }) {
    auto const mode{ PositionOf(data, element).mode };
    auto const side{ opeu_battle_element_side(game, element) };
    if (!opeu_battle_code_low(game, terrain)) opeu_battle_draw_icon(game, element, side, mode);
    opeu_battle_draw_count(game, Record<Element>(data, element).quantity, mode, side == 1);
  }
  auto const image{ static_cast<uint16_t>(animation.frames + frame * ICON_STRIDE) };
  opeu_vram_load(game.State().Bytes(Near(data, data.vram)).data(), port_far(data.icon_segment, image + ICON_BYTES));
  opeu_vram_draw(game.State().Bytes(Near(data, data.vram)).data(), port_far(data.icon_segment, image));
  opeu_battle_draw_neighbors(game, cell.x, cell.y);
  RestoreCell(game, column, row);
}
} // namespace

void opeu_battle_draw_element_at(Game& game, GridCell cell, ShownElement shown) {
  auto&      data{ Data(game) };
  auto const side{ opeu_battle_element_side(game, shown.element) };
  auto&      position{ PositionOf(data, shown.element) };
  position.x = cell.x;
  position.y = cell.y;
  opeu_battle_mark_cell(game, cell, side);
  opeu_battle_set_mode(game, shown.element, shown.mode);
  if (!opeu_battle_element_visible(game, shown.element)) return;
  auto const column{ opeu_battle_cell_column(game, cell.x, cell.y) };
  auto const row{ opeu_battle_cell_row(game, cell.x, cell.y) };
  SaveCell(game, column, row);
  data.sprite_variant[0] = TERRAIN_VARIANT;
  auto const terrain{ BattleTerrainAt(data, cell.x, cell.y) };
  opeu_battle_draw_terrain(game, cell, terrain);
  if (!opeu_battle_code_low(game, terrain)) opeu_battle_draw_icon(game, shown.element, side, shown.mode);
  opeu_battle_draw_count(game, Record<Element>(data, shown.element).quantity, shown.mode, side == 1);
  opeu_battle_draw_neighbors(game, cell.x, cell.y);
  RestoreCell(game, column, row);
}

void opeu_battle_redraw_cell(Game& game, GridCell cell, ShownElement shown) {
  if (!shown.element || !opeu_battle_element_visible(game, shown.element)) opeu_battle_draw_cell(game, cell.x, cell.y);
  if (shown.element) opeu_battle_draw_element_at(game, cell, shown);
}

void opeu_battle_refresh_cell(Game& game, uint8_t x, uint8_t y) {
  auto const element{ opeu_battle_element_at(game, x, y) };
  auto const mode{ element ? opeu_battle_mode(game, element) : uint8_t{ 0 } };
  opeu_battle_redraw_cell(game, { x, y }, { element, mode });
}

void opeu_battle_animate_element(Game& game, GridCell cell, CellAnimation const& animation) {
  DrawFrame(game, cell, animation, 0);
  opeu_battle_pause(game, 1);
  if (animation.pause) {
    opeu_battle_dispatch(game, ANIMATION_HANDLER);
    if (animation.turn) {
      opeu_effect_finish(game);
      opeu_battle_dispatch(game, ANIMATION_HANDLER);
    }
  }
  for (auto const frame : std::views::iota(uint8_t{ 1 }, ANIMATION_FRAMES)) {
    DrawFrame(game, cell, animation, frame);
    opeu_battle_pause(game, 1);
  }
}
