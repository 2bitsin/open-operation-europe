// The staff selection panel's strips, option marks and frames.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_staff_restore_bottom(Game& game, HostNotify event);
void opeu_staff_restore_left(Game& game, HostNotify event);
void opeu_staff_mark_option(Game& game, HostNotify event, int16_t option);
void opeu_staff_frame_option(Game& game, HostNotify event, int16_t option);

// The services the module calls through its event.
void svc_staffpnl(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
