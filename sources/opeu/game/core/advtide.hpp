// The staff's "Tide of War" advice.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_advice_tide_isolation(uint8_t* ds, const uint8_t* far_text, HostEvent event);
uint16_t opeu_staff_1ad2(uint8_t* ds, const uint8_t* far_text, HostEvent event);
uint16_t opeu_advice_tide_strength_balance(uint8_t* ds, const uint8_t* far_text, HostEvent event);
uint16_t opeu_advice_tide_territory_control(uint8_t* ds, const uint8_t* far_text, HostEvent event);
uint16_t opeu_advice_tide_air_superiority(uint8_t* ds, const uint8_t* far_text, HostEvent event);
uint16_t opeu_advice_tide_no_advances(uint8_t* ds, const uint8_t* far_text, HostEvent event);
void     opeu_advice_tide_of_war(uint8_t* ds, const uint8_t* far_text, HostEvent event);

// Runs the services of the module's checks.
uint16_t svc_advtide(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
