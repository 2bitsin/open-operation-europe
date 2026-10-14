// Dated records that are due, and an objective's level.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

#include <cstdint>
#include <vector>

auto    opeu_list_due_records(Game const& game, uint8_t mask, uint8_t kind) -> std::vector<std::uint8_t>;
uint8_t opeu_objective_level(Game const& game, uint16_t objective);
