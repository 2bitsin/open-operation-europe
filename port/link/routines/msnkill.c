/* Routines of core/msnkill.c reached by identifier: the assassination
 * handler 0e8e:06f8 of the DS:7a54 far table (data/dgevents.c), called by
 * msntick service 17 with args[0] = side. */
#include "core/c2_common.h"

PORT_ROUTINE(opeu_mission_assassinate)
{
    uint16_t frame = c2_frame_enter();
    opeu_mission_assassinate(ds, link_msnkill, frame, (uint8_t)ARG(0));
    c2_frame_leave();
    return 0;
}
