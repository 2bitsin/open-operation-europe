// Sister Angelique's visit to a unit with a general in poor health or veteran battalions.
#pragma once

#include "types.hpp"
#include "common/events.hpp"

uint16_t opeu_angel_unit_eligible(uint8_t* ds, HostEvent event, uint16_t unit);
uint8_t  opeu_angel_portrait(uint8_t* ds, HostEvent event, uint8_t side);
void     opeu_angel_finish(uint8_t* ds, HostEvent event, uint16_t person,
                           uint16_t unit);
void     opeu_angel_demoralize(uint8_t* ds, HostEvent event, uint16_t person,
                               uint16_t unit);
void     opeu_angel_inspire(uint8_t* ds, HostEvent event, uint16_t person,
                            uint16_t unit);
void     opeu_angel_judge_side(uint8_t* ds, HostEvent event, uint16_t person,
                               uint16_t unit);
void opeu_angel_present(uint8_t* ds, HostEvent event, uint16_t code_segment);
void opeu_angel_event(uint8_t* ds, HostEvent event, uint16_t code_segment);

// Runs the services the module's routines call through their event.
uint16_t svc_evangel(uint8_t* ds, uint16_t id, const uint16_t* args,
                     uint16_t count);
