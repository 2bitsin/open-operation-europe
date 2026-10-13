// The staff candidates' ratings and the selected candidate's portrait.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t opeu_staff_jitter_stat(uint8_t v);
void    opeu_staff_roll_flag(Game& game, uint16_t person);
void    opeu_staff_reroll_person(Game& game, uint8_t index);
uint8_t opeu_staff_reroll_selected(Game& game, HostNotify event);

// The services the module calls through its event.
void svc_staffgen(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
