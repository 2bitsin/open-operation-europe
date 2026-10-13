#pragma once

#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/grid-cell.hpp"
#include "common/map-frame.hpp"
#include "types.hpp"

// The overview's and the tactical view's pixel position of a map cell (x counts 8-pixel byte columns).
[[nodiscard]] auto OverviewPixel(opeu::game::GridCell cell) -> opeu::game::ScreenPoint;
[[nodiscard]] auto ViewPixel(opeu::game::DataSegment const& data, opeu::game::GridCell cell) -> opeu::game::ScreenPoint;

void     opeu_sprite_recolor(Game& game, uint16_t data, uint16_t size, uint8_t side);
uint16_t opeu_unit_visible(Game& game, uint16_t unit);
