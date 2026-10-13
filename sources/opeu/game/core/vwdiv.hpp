// Contacting a division from the view menu.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_view_division_contact(Game& game, HostEvent event);

// The services of the division contact (the information views' shared services).
uint16_t svc_vwdiv(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
