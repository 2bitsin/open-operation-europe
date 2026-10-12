#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t opeu_group_target_enabled(uint8_t* ds);
uint8_t opeu_menu_always(void);
void    opeu_group_set_target(uint8_t* ds, HostEvent event, uint16_t sp, uint8_t group);

// The services of this module.
uint16_t svc_ordtgt(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
