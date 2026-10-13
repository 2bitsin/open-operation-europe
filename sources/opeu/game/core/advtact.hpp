// The staff's tactical advice.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_advice_tactical_objective_threat(Game& game, const uint8_t* far_text, HostEvent event);
uint16_t opeu_advice_tactical_command(Game& game, const uint8_t* far_text, HostEvent event);
uint16_t opeu_advice_tactical_reorganize(Game& game, const uint8_t* far_text, HostEvent event);
uint16_t opeu_advice_tactical_unit_deployment(Game& game, const uint8_t* far_text, HostEvent event);
uint16_t opeu_side_objective_slot_pending(Game& game, const uint8_t* far_text, uint8_t slot);
uint16_t opeu_side_request_kind_is(Game& game, const uint8_t* far_text, uint8_t kind);
uint16_t opeu_advice_tactical_supply_reinforce(Game& game, const uint8_t* far_text, HostEvent event);
uint16_t opeu_advice_tactical_land_mines(Game& game, const uint8_t* far_text, HostEvent event);
uint8_t  opeu_ready_group_has_idle_unit(Game& game, const uint8_t* far_text);
uint16_t opeu_advice_tactical_air_special_forces(Game& game, const uint8_t* far_text, HostEvent event);
uint16_t opeu_advice_tactical_linked_objective(Game& game, const uint8_t* far_text, HostEvent event);
void     opeu_advice_tactical(Game& game, const uint8_t* far_text, HostEvent event);

// Runs the services of the module's checks.
uint16_t svc_advtact(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
