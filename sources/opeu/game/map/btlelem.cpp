// Battle elements on the map: placing and drawing one on its cell, redrawing a cell, and animating an element.
//
// Each routine that reaches the icon drawing takes its frame on the DS stack and hands its callee the frame below.
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

#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
// The frames the callees run on, below each routine's frame.
constexpr uint16_t AT_ICON_FRAME{ 0x1c };
constexpr uint16_t FRAME_ICON_FRAME{ 0x1e };
constexpr uint16_t REDRAW_AT_FRAME{ 0x0e };
constexpr uint16_t REFRESH_REDRAW_FRAME{ 0x10 };
constexpr uint16_t ANIMATE_FRAME_FRAME{ 0x12 };
// Sprite_variant while a cell's terrain is drawn under an element.
constexpr uint8_t  TERRAIN_VARIANT{ 2 };
constexpr uint16_t ICON_BYTES{ 520 };
// An animation frame: a bitmap and its mask.
constexpr uint16_t ICON_STRIDE{ 650 };
constexpr uint8_t  ANIMATION_FRAMES{ 4 };
constexpr uint8_t  ANIMATION_HANDLER{ 5 };

// Saves (and restores) the screen under a cell; a column not on a byte boundary takes the odd routine.
void SaveCell(uint8_t* ds, BattleEvent event, uint16_t column, uint16_t row) {
  Call(ds, event, column & 7 ? BattleRoutine::VRAM_SAVE_ODD : BattleRoutine::VRAM_SAVE, column >> 3, row, Near(Data(ds), Data(ds).vram));
}
void RestoreCell(uint8_t* ds, BattleEvent event, uint16_t column, uint16_t row) {
  Call(ds, event, column & 7 ? BattleRoutine::VRAM_RESTORE_ODD : BattleRoutine::VRAM_RESTORE, column >> 3, row, Near(Data(ds), Data(ds).vram));
}
} // namespace

void opeu_battle_draw_element_at(uint8_t* ds, BattleEvent event, uint8_t x, uint8_t y, uint16_t element, uint8_t mode, uint16_t frame) {
  auto&      data{ Data(ds) };
  auto const side{ opeu_battle_element_side(ds, event, element) };
  auto&      position{ PositionOf(data, element) };
  position.x = x;
  position.y = y;
  opeu_battle_mark_cell(ds, event, position.x, position.y, side);
  opeu_battle_set_mode(ds, event, element, mode);
  if (!opeu_battle_element_visible(ds, event, element)) return;
  auto const column{ opeu_battle_cell_column(ds, event, x, y) };
  auto const row{ opeu_battle_cell_row(ds, event, x, y) };
  SaveCell(ds, event, column, row);
  data.sprite_variant[0] = TERRAIN_VARIANT;
  auto const terrain{ BattleTerrainAt(data, x, y) };
  opeu_battle_draw_terrain(ds, event, x, y, terrain);
  if (!opeu_battle_code_low(ds, event, terrain)) opeu_battle_draw_icon(ds, event, element, side, mode, Local(frame, AT_ICON_FRAME));
  opeu_battle_draw_count(ds, event, Record<Element>(data, element).quantity, mode, side == 1);
  opeu_battle_draw_neighbors(ds, event, x, y);
  RestoreCell(ds, event, column, row);
}

void opeu_battle_redraw_cell(uint8_t* ds, BattleEvent event, uint8_t x, uint8_t y, uint16_t element, uint8_t mode, uint16_t frame) {
  if (!element || !opeu_battle_element_visible(ds, event, element)) opeu_battle_draw_cell(ds, event, x, y);
  if (element) opeu_battle_draw_element_at(ds, event, x, y, element, mode, Local(frame, REDRAW_AT_FRAME));
}

void opeu_battle_refresh_cell(uint8_t* ds, BattleEvent event, uint8_t x, uint8_t y, uint16_t frame) {
  auto const element{ Call(ds, event, BattleRoutine::BATTLE_ELEMENT_AT, x, y) };
  auto const mode{ element ? opeu_battle_mode(ds, event, element) : 0 };
  opeu_battle_redraw_cell(ds, event, x, y, element, static_cast<uint8_t>(mode), Local(frame, REFRESH_REDRAW_FRAME));
}

// offset: the animation's first frame in the icon segment.
void opeu_battle_draw_element_frame(uint8_t* ds, BattleEvent event, uint8_t x, uint8_t y, uint16_t offset, uint8_t frame_index, uint16_t element, uint16_t frame) {
  auto&      data{ Data(ds) };
  auto const column{ opeu_battle_cell_column(ds, event, x, y) };
  auto const row{ opeu_battle_cell_row(ds, event, x, y) };
  SaveCell(ds, event, column, row);
  data.sprite_variant[0] = TERRAIN_VARIANT;
  auto const terrain{ BattleTerrainAt(data, x, y) };
  opeu_battle_draw_terrain(ds, event, x, y, terrain);
  if (element) {
    auto const mode{ PositionOf(data, element).mode };
    auto const side{ opeu_battle_element_side(ds, event, element) };
    if (!opeu_battle_code_low(ds, event, terrain)) opeu_battle_draw_icon(ds, event, element, side, mode, Local(frame, FRAME_ICON_FRAME));
    opeu_battle_draw_count(ds, event, Record<Element>(data, element).quantity, mode, side == 1);
  }
  auto const image{ static_cast<uint16_t>(offset + frame_index * ICON_STRIDE) };
  Call(ds, event, BattleRoutine::VRAM_LOAD, Near(data, data.vram), data.icon_segment, image + ICON_BYTES);
  Call(ds, event, BattleRoutine::VRAM_DRAW, Near(data, data.vram), data.icon_segment, image);
  opeu_battle_draw_neighbors(ds, event, x, y);
  RestoreCell(ds, event, column, row);
}

// turn and pause: after the first frame, run the animation handler, and with turn the turn service and the handler
// again.
void opeu_battle_animate_element(uint8_t* ds, BattleEvent event, uint8_t x, uint8_t y, uint16_t turn, uint16_t pause, uint16_t offset, uint16_t element, uint16_t frame) {
  opeu_battle_draw_element_frame(ds, event, x, y, offset, 0, element, Local(frame, ANIMATE_FRAME_FRAME));
  Call(ds, event, BattleRoutine::BATTLE_PAUSE, 1);
  if (pause) {
    opeu_battle_dispatch(ds, event, ANIMATION_HANDLER);
    if (turn) {
      Call(ds, event, BattleRoutine::EFFECT_FINISH);
      opeu_battle_dispatch(ds, event, ANIMATION_HANDLER);
    }
  }
  for (uint8_t i{ 1 }; i < ANIMATION_FRAMES; ++i) {
    opeu_battle_draw_element_frame(ds, event, x, y, offset, i, element, Local(frame, ANIMATE_FRAME_FRAME));
    Call(ds, event, BattleRoutine::BATTLE_PAUSE, 1);
  }
}

uint16_t svc_btlelem(uint8_t* ds, uint32_t address, const uint16_t* args, unsigned int count) {
  return port_service(ds, address, args, count);
}

auto port_routine_opeu_battle_animate_element(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  WithBattleStack([&](uint16_t sp) {
    opeu_battle_animate_element(ds, svc_btlelem, static_cast<uint8_t>(Arg(args, count, 0)), static_cast<uint8_t>(Arg(args, count, 1)), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4),
                                Arg(args, count, 5), BattleFrame(sp));
  });
  return 0;
}

auto port_routine_opeu_battle_draw_element_at(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  WithBattleStack([&](uint16_t sp) {
    opeu_battle_draw_element_at(ds, svc_btlelem, static_cast<uint8_t>(Arg(args, count, 0)), static_cast<uint8_t>(Arg(args, count, 1)), Arg(args, count, 2), static_cast<uint8_t>(Arg(args, count, 3)),
                                BattleFrame(sp));
  });
  return 0;
}

auto port_routine_opeu_battle_redraw_cell(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  WithBattleStack([&](uint16_t sp) {
    opeu_battle_redraw_cell(ds, svc_btlelem, static_cast<uint8_t>(Arg(args, count, 0)), static_cast<uint8_t>(Arg(args, count, 1)), Arg(args, count, 2), static_cast<uint8_t>(Arg(args, count, 3)),
                            BattleFrame(sp));
  });
  return 0;
}

auto port_routine_opeu_battle_refresh_cell(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  WithBattleStack([&](uint16_t sp) { opeu_battle_refresh_cell(ds, svc_btlelem, static_cast<uint8_t>(Arg(args, count, 0)), static_cast<uint8_t>(Arg(args, count, 1)), BattleFrame(sp)); });
  return 0;
}
