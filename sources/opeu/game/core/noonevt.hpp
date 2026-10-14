// The dated noon events, their actions and the leaders' speeches.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

#include <span>

void opeu_turn_noon_events(Game& game, std::span<std::uint8_t const> far_text);
