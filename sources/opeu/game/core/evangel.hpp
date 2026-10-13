// Sister Angelique's visit to a unit with a general in poor health or veteran battalions.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_angel_unit_eligible(Game& game, uint16_t unit);
uint8_t  opeu_angel_portrait(Game& game, HostEvent event, uint8_t side);
void     opeu_angel_finish(Game& game, uint16_t person, uint16_t unit);
void     opeu_angel_demoralize(Game& game, uint16_t person, uint16_t unit);
void     opeu_angel_inspire(Game& game, uint16_t person, uint16_t unit);
void     opeu_angel_judge_side(Game& game, uint16_t person, uint16_t unit);
void     opeu_angel_present(Game& game, HostEvent event, uint16_t code_segment);
void     opeu_angel_event(Game& game, HostEvent event, uint16_t code_segment);

// Runs the services the module's routines call through their event.
uint16_t svc_evangel(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
