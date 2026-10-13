// Converts the record indices a scenario or saved game stores into near pointers.
#include "common/events.hpp"
#pragma once

#include <cstdint>

void opeu_rebase_scenario(Game& game, std::uint16_t extended);
