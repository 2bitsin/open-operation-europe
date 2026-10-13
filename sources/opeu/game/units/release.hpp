// Releasing the current side's elements and regiments.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t opeu_release_side_elements(Game& game, uint16_t list, uint8_t mask);
uint8_t opeu_release_side_units(Game& game, uint16_t list, uint8_t mask);
