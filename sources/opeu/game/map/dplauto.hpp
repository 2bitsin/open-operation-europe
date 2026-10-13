#pragma once

#include "common/events.hpp"
#include "common/grid-cell.hpp"
#include "types.hpp"

#include <cstdint>
#include <optional>

// The first free cell of the element's deployment template entry `index` (for its unit's approach direction).
[[nodiscard]] auto opeu_deploy_template_cell(Game& game, uint16_t element, uint8_t index) -> std::optional<opeu::game::GridCell>;
// The first template entry's free cell, if any.
[[nodiscard]] auto opeu_deploy_by_template(Game& game, uint16_t element) -> std::optional<opeu::game::GridCell>;
// A cell in the attacker's scan rectangle, else on clear terrain, else anywhere by chance.
[[nodiscard]] auto opeu_deploy_by_scan(Game& game, uint16_t element) -> opeu::game::GridCell;
// A cell in the field centre by chance, else anywhere by chance.
[[nodiscard]] auto opeu_deploy_at_random(Game& game, uint16_t element) -> opeu::game::GridCell;
void               opeu_deploy_put_at(Game& game, uint16_t element, opeu::game::GridCell cell);
void               opeu_deploy_garrison(Game& game, uint16_t unit);
void               opeu_deploy_auto(Game& game, uint16_t unit);
