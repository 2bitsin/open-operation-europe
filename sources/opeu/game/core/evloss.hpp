// Hourly unit losses: partisan and resistance strikes, and the losses north of row 10.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_loss_unit_weak(Game& game, uint16_t unit);
void     opeu_loss_present_a(Game& game);
void     opeu_loss_event_a(Game& game);
uint16_t opeu_loss_unit_low(Game& game, uint16_t unit);
void     opeu_loss_present_b(Game& game);
void     opeu_loss_event_b(Game& game);
