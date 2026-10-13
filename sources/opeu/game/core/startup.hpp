// Starting a game: staffing the computer sides and the start-up sequence.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_person_ability_total(Game const& game, uint16_t person);
uint8_t  opeu_rank_side_people(Game& game, uint8_t side, uint8_t* order);
void     opeu_assign_side_staff(Game& game);
void     opeu_app_startup_service(Game& game, HostEvent event);

// The services opeu_app_startup_service is called with.
uint16_t svc_startup(Game& game, uint16_t service, const uint16_t* args, uint16_t count);
