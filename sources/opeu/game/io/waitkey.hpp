// Timed waits that poll the keyboard.
#pragma once

#include "common/events.hpp"

#include <cstdint>

void opeu_delay_ticks(Game& game, std::uint16_t ticks);
void opeu_delay_units(Game& game, std::uint8_t units);
void opeu_pending_finish(Game& game, std::uint8_t value);
