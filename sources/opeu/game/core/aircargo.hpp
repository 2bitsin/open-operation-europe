// The supply airlift's flight, and the hourly air operations.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_cargo_drop_due(Game& game);
void     opeu_cargo_distribute(Game& game, uint16_t cargo, uint16_t amount);
void     opeu_cargo_drop_present(Game& game);
void     opeu_cargo_drop_event(Game& game);
void     opeu_air_operations(Game& game);
