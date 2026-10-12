// The staff selection panel's strips, option marks and frames.
#pragma once

#include "types.hpp"
#include "common/events.hpp"

void opeu_staff_restore_bottom(uint8_t* ds, HostNotify event);
void opeu_staff_restore_left(uint8_t* ds, HostNotify event);
void opeu_staff_mark_option(uint8_t* ds, HostNotify event, int16_t option);
void opeu_staff_frame_option(uint8_t* ds, HostNotify event, int16_t option);

// The services the module calls through its event.
void svc_staffpnl(uint8_t* ds, uint16_t id, const uint16_t* args,
                  uint16_t count);
