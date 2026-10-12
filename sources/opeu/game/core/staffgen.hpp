// The staff candidates' ratings and the selected candidate's portrait.
#pragma once

#include "types.hpp"
#include "types/events.hpp"

uint8_t opeu_staff_jitter_stat(uint8_t v);
void    opeu_staff_roll_flag(uint8_t* ds, uint16_t person);
void    opeu_staff_reroll_person(uint8_t* ds, uint8_t index);
uint8_t opeu_staff_reroll_selected(uint8_t* ds, HostNotify event);

// The services the module calls through its event.
void svc_staffgen(uint8_t* ds, uint16_t id, const uint16_t* args,
                  uint16_t count);
