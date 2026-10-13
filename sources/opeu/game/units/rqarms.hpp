// The battalion (arms) request of the request menu, and the services the menu's requests share.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

#include <cstdint>

void    opeu_request_arms(Game& game);
uint8_t opeu_rq_unit_selectable(void);
