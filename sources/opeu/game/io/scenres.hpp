// Loads a scenario's text, the message pictures and the scenario's map, frame, tile and font resources.
#pragma once

#include "common/events.hpp"
#include "common/text.hpp"

#include <cstdint>

void opeu_load_scenario_text(Game& game, std::uint8_t number, TextTarget destination);
void opeu_load_message_graphics(Game& game);
void opeu_load_scenario_graphics(Game& game);
