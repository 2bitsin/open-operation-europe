// Battle elements on the map: placing and drawing one on its cell, redrawing a cell, and animating an element.
#include "map/btlelem.hpp"
#include "common/battle-field.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "map/btlcell.hpp"
#include "map/btldraw.hpp"
#include "map/btlicon.hpp"
#include "map/btllist.hpp"
#include "map/btlsprt.hpp"
#include "map/btlunit.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "link/link.hpp"
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
void SaveCell(Game& game, BattleEvent event, uint16_t column, uint16_t row) {
  Call(game, event, column & 7 ? BattleRoutine::VRAM_SAVE_ODD : BattleRoutine::VRAM_SAVE, column >> 3, row, Near(Data(game), Data(game).vram));
}
void RestoreCell(Game& game, BattleEvent event, uint16_t column, uint16_t row) {
  Call(game, event, column & 7 ? BattleRoutine::VRAM_RESTORE_ODD : BattleRoutine::VRAM_RESTORE, column >> 3, row, Near(Data(game), Data(game).vram));
}
} // namespace

void opeu_battle_draw_element_at(Game& game, BattleEvent event, uint8_t x, uint8_t y, uint16_t element, uint8_t mode) {
  auto&      data{ Data(game) };
  auto const side{ opeu_battle_element_side(game, event, element) };
  auto&      position{ PositionOf(data, element) };
  position.x = x;
  position.y = y;
  opeu_battle_mark_cell(game, event, position.x, position.y, side);
  opeu_battle_set_mode(game, event, element, mode);
  if (!opeu_battle_element_visible(game, event, element)) return;
  auto const column{ opeu_battle_cell_column(game, event, x, y) };
  auto const row{ opeu_battle_cell_row(game, event, x, y) };
  SaveCell(game, event, column, row);
  data.sprite_variant[0] = TERRAIN_VARIANT;
  auto const terrain{ BattleTerrainAt(data, x, y) };
  opeu_battle_draw_terrain(game, event, x, y, terrain);
  if (!opeu_battle_code_low(game, event, terrain)) opeu_battle_draw_icon(game, event, element, side, mode);
  opeu_battle_draw_count(game, event, Record<Element>(data, element).quantity, mode, side == 1);
  opeu_battle_draw_neighbors(game, event, x, y);
  RestoreCell(game, event, column, row);
}

void opeu_battle_redraw_cell(Game& game, BattleEvent event, uint8_t x, uint8_t y, uint16_t element, uint8_t mode) {
  if (!element || !opeu_battle_element_visible(game, event, element)) opeu_battle_draw_cell(game, event, x, y);
  if (element) opeu_battle_draw_element_at(game, event, x, y, element, mode);
}

void opeu_battle_refresh_cell(Game& game, BattleEvent event, uint8_t x, uint8_t y) {
  auto const element{ Call(game, event, BattleRoutine::BATTLE_ELEMENT_AT, x, y) };
  auto const mode{ element ? opeu_battle_mode(game, event, element) : 0 };
  opeu_battle_redraw_cell(game, event, x, y, element, static_cast<uint8_t>(mode));
}

// offset: the animation's first frame in the icon segment.
void opeu_battle_draw_element_frame(Game& game, BattleEvent event, uint8_t x, uint8_t y, uint16_t offset, uint8_t frame_index, uint16_t element) {
  auto&      data{ Data(game) };
  auto const column{ opeu_battle_cell_column(game, event, x, y) };
  auto const row{ opeu_battle_cell_row(game, event, x, y) };
  SaveCell(game, event, column, row);
  data.sprite_variant[0] = TERRAIN_VARIANT;
  auto const terrain{ BattleTerrainAt(data, x, y) };
  opeu_battle_draw_terrain(game, event, x, y, terrain);
  if (element) {
    auto const mode{ PositionOf(data, element).mode };
    auto const side{ opeu_battle_element_side(game, event, element) };
    if (!opeu_battle_code_low(game, event, terrain)) opeu_battle_draw_icon(game, event, element, side, mode);
    opeu_battle_draw_count(game, event, Record<Element>(data, element).quantity, mode, side == 1);
  }
  auto const image{ static_cast<uint16_t>(offset + frame_index * ICON_STRIDE) };
  Call(game, event, BattleRoutine::VRAM_LOAD, Near(data, data.vram), data.icon_segment, image + ICON_BYTES);
  Call(game, event, BattleRoutine::VRAM_DRAW, Near(data, data.vram), data.icon_segment, image);
  opeu_battle_draw_neighbors(game, event, x, y);
  RestoreCell(game, event, column, row);
}

// turn and pause: after the first frame, run the animation handler, and with turn the turn service and the handler
// again.
void opeu_battle_animate_element(Game& game, BattleEvent event, uint8_t x, uint8_t y, uint16_t turn, uint16_t pause, uint16_t offset, uint16_t element) {
  opeu_battle_draw_element_frame(game, event, x, y, offset, 0, element);
  Call(game, event, BattleRoutine::BATTLE_PAUSE, 1);
  if (pause) {
    opeu_battle_dispatch(game, event, ANIMATION_HANDLER);
    if (turn) {
      Call(game, event, BattleRoutine::EFFECT_FINISH);
      opeu_battle_dispatch(game, event, ANIMATION_HANDLER);
    }
  }
  for (uint8_t i{ 1 }; i < ANIMATION_FRAMES; ++i) {
    opeu_battle_draw_element_frame(game, event, x, y, offset, i, element);
    Call(game, event, BattleRoutine::BATTLE_PAUSE, 1);
  }
}

uint16_t svc_btlelem(Game& game, uint32_t address, const uint16_t* args, unsigned int count) {
  return port_service(game, address, args, count);
}

auto port_routine_opeu_battle_animate_element(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_battle_animate_element(game, svc_btlelem, static_cast<uint8_t>(Arg(args, count, 0)), static_cast<uint8_t>(Arg(args, count, 1)), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4),
                              Arg(args, count, 5));
  return 0;
}

auto port_routine_opeu_battle_draw_element_at(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_battle_draw_element_at(game, svc_btlelem, static_cast<uint8_t>(Arg(args, count, 0)), static_cast<uint8_t>(Arg(args, count, 1)), Arg(args, count, 2), static_cast<uint8_t>(Arg(args, count, 3)));
  return 0;
}

auto port_routine_opeu_battle_redraw_cell(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_battle_redraw_cell(game, svc_btlelem, static_cast<uint8_t>(Arg(args, count, 0)), static_cast<uint8_t>(Arg(args, count, 1)), Arg(args, count, 2), static_cast<uint8_t>(Arg(args, count, 3)));
  return 0;
}

auto port_routine_opeu_battle_refresh_cell(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_battle_refresh_cell(game, svc_btlelem, static_cast<uint8_t>(Arg(args, count, 0)), static_cast<uint8_t>(Arg(args, count, 1)));
  return 0;
}
