// Starting a game: staffing the computer sides and the start-up sequence.
#pragma once

#include "types.hpp"
#include "common/events.hpp"

uint16_t opeu_person_ability_total(const uint8_t* ds, uint16_t person);
uint8_t  opeu_rank_side_people(uint8_t* ds, uint8_t side, uint8_t* order);
void     opeu_assign_side_staff(uint8_t* ds);
void     opeu_app_startup_service(uint8_t* ds, HostEvent event);

// The services opeu_app_startup_service is called with.
uint16_t svc_startup(uint8_t* ds, uint16_t service, const uint16_t* args,
                     uint16_t count);
