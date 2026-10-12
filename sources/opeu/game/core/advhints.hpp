// The staff's operation hints and general tips.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_advice_situational_hint(uint8_t* ds, const uint8_t* far_text, HostEvent event, uint16_t sp);
uint16_t opeu_advice_general_tip(uint8_t* ds, const uint8_t* far_text, HostEvent event, uint16_t sp);
uint16_t opeu_advice_operation_hints(uint8_t* ds, const uint8_t* far_text, HostEvent event, uint16_t sp);

// Runs the services of the module's checks.
uint16_t svc_advhints(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
