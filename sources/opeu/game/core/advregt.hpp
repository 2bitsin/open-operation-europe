// The staff's "Regiment Status" advice.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_advice_regiment_order_finishing(Game& game, const uint8_t* far_text, HostEvent event);
uint16_t opeu_advice_regiment_supply_shortage(Game& game, const uint8_t* far_text, HostEvent event);
uint16_t opeu_advice_division_commander_condition(Game& game, const uint8_t* far_text, HostEvent event);
uint16_t opeu_advice_regiment_damage_or_comms(Game& game, const uint8_t* far_text, HostEvent event);
uint16_t opeu_advice_regiment_fatigue_morale(Game& game, const uint8_t* far_text, HostEvent event);
uint16_t opeu_advice_regiment_nothing(Game& game, const uint8_t* far_text, HostEvent event);
uint16_t opeu_advice_regiment_status(Game& game, const uint8_t* far_text, HostEvent event);

// Runs the services of the module's checks.
uint16_t svc_advregt(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
