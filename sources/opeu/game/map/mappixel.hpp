#pragma once

#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/grid-cell.hpp"
#include "common/map-frame.hpp"
#include "types.hpp"

#include <cstdint>
#include <span>

// The overview's and the tactical view's pixel position of a map cell (x counts 8-pixel byte columns).
[[nodiscard]] auto OverviewPixel(opeu::game::GridCell cell) -> opeu::game::ScreenPoint;
[[nodiscard]] auto ViewPixel(opeu::game::DataSegment const& data, opeu::game::GridCell cell) -> opeu::game::ScreenPoint;

void     opeu_sprite_recolor(std::span<std::uint8_t> sprite, uint8_t side);
uint16_t opeu_unit_visible(Game& game, uint16_t unit);
