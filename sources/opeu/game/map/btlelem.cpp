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
} // namespace

void opeu_battle_draw_element_at(Game& game, uint8_t x, uint8_t y, uint16_t element, uint8_t mode) {
  auto&      data{ Data(game) };
  auto const side{ opeu_battle_element_side(game, element) };
  auto&      position{ PositionOf(data, element) };
  position.x = x;
  position.y = y;
  opeu_battle_mark_cell(game, position.x, position.y, side);
  opeu_battle_set_mode(game, element, mode);
  if (!opeu_battle_element_visible(game, element)) return;
  auto const column{ opeu_battle_cell_column(game, x, y) };
  auto const row{ opeu_battle_cell_row(game, x, y) };
  SaveCell(game, column, row);
  data.sprite_variant[0] = TERRAIN_VARIANT;
  auto const terrain{ BattleTerrainAt(data, x, y) };
  opeu_battle_draw_terrain(game, x, y, terrain);
  if (!opeu_battle_code_low(game, terrain)) opeu_battle_draw_icon(game, element, side, mode);
  opeu_battle_draw_count(game, Record<Element>(data, element).quantity, mode, side == 1);
  opeu_battle_draw_neighbors(game, x, y);
  RestoreCell(game, column, row);
}

void opeu_battle_redraw_cell(Game& game, uint8_t x, uint8_t y, uint16_t element, uint8_t mode) {
  if (!element || !opeu_battle_element_visible(game, element)) opeu_battle_draw_cell(game, x, y);
  if (element) opeu_battle_draw_element_at(game, x, y, element, mode);
}

void opeu_battle_refresh_cell(Game& game, uint8_t x, uint8_t y) {
  auto const element{ opeu_battle_element_at(game, static_cast<uint8_t>(x), static_cast<uint8_t>(y)) };
  auto const mode{ element ? opeu_battle_mode(game, element) : 0 };
  opeu_battle_redraw_cell(game, x, y, element, static_cast<uint8_t>(mode));
}

// offset: the animation's first frame in the icon segment.
void opeu_battle_draw_element_frame(Game& game, uint8_t x, uint8_t y, uint16_t offset, uint8_t frame_index, uint16_t element) {
  auto&      data{ Data(game) };
  auto const column{ opeu_battle_cell_column(game, x, y) };
  auto const row{ opeu_battle_cell_row(game, x, y) };
  SaveCell(game, column, row);
  data.sprite_variant[0] = TERRAIN_VARIANT;
  auto const terrain{ BattleTerrainAt(data, x, y) };
  opeu_battle_draw_terrain(game, x, y, terrain);
  if (element) {
    auto const mode{ PositionOf(data, element).mode };
    auto const side{ opeu_battle_element_side(game, element) };
    if (!opeu_battle_code_low(game, terrain)) opeu_battle_draw_icon(game, element, side, mode);
    opeu_battle_draw_count(game, Record<Element>(data, element).quantity, mode, side == 1);
  }
  auto const image{ static_cast<uint16_t>(offset + frame_index * ICON_STRIDE) };
  opeu_vram_load(game.State().Bytes(Near(data, data.vram)).data(), port_far(data.icon_segment, image + ICON_BYTES));
  opeu_vram_draw(game.State().Bytes(Near(data, data.vram)).data(), port_far(data.icon_segment, image));
  opeu_battle_draw_neighbors(game, x, y);
  RestoreCell(game, column, row);
}

// turn and pause: after the first frame, run the animation handler, and with turn the turn service and the handler
// again.
void opeu_battle_animate_element(Game& game, uint8_t x, uint8_t y, uint16_t turn, uint16_t pause, uint16_t offset, uint16_t element) {
  opeu_battle_draw_element_frame(game, x, y, offset, 0, element);
  opeu_battle_pause(game, svc_btlcalc, static_cast<uint8_t>(1));
  if (pause) {
    opeu_battle_dispatch(game, ANIMATION_HANDLER);
    if (turn) {
      opeu_effect_finish(game);
      opeu_battle_dispatch(game, ANIMATION_HANDLER);
    }
  }
  for (uint8_t i{ 1 }; i < ANIMATION_FRAMES; ++i) {
    opeu_battle_draw_element_frame(game, x, y, offset, i, element);
    opeu_battle_pause(game, svc_btlcalc, static_cast<uint8_t>(1));
  }
}
