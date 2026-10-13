#pragma once

#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/grid-cell.hpp"
#include "types.hpp"

#include <optional>

[[nodiscard]] auto opeu_battle_cell_open(Game& game, opeu::game::GridCell cell, uint16_t element) -> uint16_t;
void               opeu_battle_cursor_draw(Game& game);
void               opeu_battle_cursor_step(Game& game, int16_t dx, int16_t dy);
// The field cell under a screen pixel; none off the field.
[[nodiscard]] auto opeu_battle_mouse_cell(opeu::game::ScreenPoint pixel) -> std::optional<opeu::game::GridCell>;
