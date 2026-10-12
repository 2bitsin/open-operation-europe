// The staff's tactical advice.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_advice_tactical_objective_threat(uint8_t* ds, const uint8_t* far_text, HostEvent event, uint16_t sp);
uint16_t opeu_advice_tactical_command(uint8_t* ds, const uint8_t* far_text, HostEvent event, uint16_t sp);
uint16_t opeu_advice_tactical_reorganize(uint8_t* ds, const uint8_t* far_text, HostEvent event, uint16_t sp);
uint16_t opeu_advice_tactical_unit_deployment(uint8_t* ds, const uint8_t* far_text, HostEvent event, uint16_t sp);
uint16_t opeu_side_objective_slot_pending(uint8_t* ds, const uint8_t* far_text, HostEvent event, uint16_t sp,
                                          uint8_t slot);
uint16_t opeu_side_request_kind_is(uint8_t* ds, const uint8_t* far_text, HostEvent event, uint16_t sp, uint8_t kind);
uint16_t opeu_advice_tactical_supply_reinforce(uint8_t* ds, const uint8_t* far_text, HostEvent event, uint16_t sp);
uint16_t opeu_advice_tactical_land_mines(uint8_t* ds, const uint8_t* far_text, HostEvent event, uint16_t sp);
uint8_t  opeu_ready_group_has_idle_unit(uint8_t* ds, const uint8_t* far_text, HostEvent event, uint16_t sp);
uint16_t opeu_advice_tactical_air_special_forces(uint8_t* ds, const uint8_t* far_text, HostEvent event, uint16_t sp);
uint16_t opeu_advice_tactical_linked_objective(uint8_t* ds, const uint8_t* far_text, HostEvent event, uint16_t sp);
void     opeu_advice_tactical(uint8_t* ds, const uint8_t* far_text, HostEvent event, uint16_t sp);

// Runs the services of the module's checks.
uint16_t svc_advtact(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
