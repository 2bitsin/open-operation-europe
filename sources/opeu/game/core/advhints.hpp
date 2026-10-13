// The staff's operation hints and general tips.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_advice_situational_hint(Game& game, const uint8_t* far_text, HostEvent event);
uint16_t opeu_advice_general_tip(Game& game, const uint8_t* far_text, HostEvent event);
uint16_t opeu_advice_operation_hints(Game& game, const uint8_t* far_text, HostEvent event);

// Runs the services of the module's checks.
uint16_t svc_advhints(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
