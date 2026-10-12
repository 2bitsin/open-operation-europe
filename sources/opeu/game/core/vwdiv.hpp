// Contacting a division from the view menu.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_view_division_contact(uint8_t* ds, HostEvent event, uint16_t sp);

// The services of the division contact (the information views' shared services).
uint16_t svc_vwdiv(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
