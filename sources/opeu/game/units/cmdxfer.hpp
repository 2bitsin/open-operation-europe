// Turn action 3: transfer of command.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_transfer_hours(uint8_t* ds, uint16_t from, uint16_t to, uint16_t person);
uint8_t  opeu_transfer_group_enabled(uint8_t* ds);
uint8_t  opeu_transfer_unit_enabled(uint8_t* ds);
uint8_t  opeu_transfer_element_enabled(uint8_t* ds);
uint8_t  opeu_transfer_units_fill(uint8_t* ds);
uint8_t  opeu_transfer_elements_fill(uint8_t* ds);
void     opeu_transfer_present(uint8_t* ds, HostEvent event);
void     opeu_turn_action_3(uint8_t* ds, HostEvent event, uint16_t sp, uint16_t segment);

uint16_t svc_cmdxfer(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
