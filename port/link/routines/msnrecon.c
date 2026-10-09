/* Routines of core/msnrecon.c reached by identifier: the reconnaissance
 * handlers 0e8e:0910 and 0e8e:09b8 of the DS:7a54 far table
 * (data/dgevents.c), called by msntick service 17 with args[0] = side. */
#include "core/c2_common.h"

PORT_ROUTINE(opeu_mission_recon_objective)
{
    uint16_t frame = c2_frame_enter();
    opeu_mission_recon_objective(ds, link_msnrecon, frame, (uint8_t)ARG(0));
    c2_frame_leave();
    return 0;
}

PORT_ROUTINE(opeu_mission_recon_unit)
{
    uint16_t frame = c2_frame_enter();
    opeu_mission_recon_unit(ds, link_msnrecon, frame, (uint8_t)ARG(0));
    c2_frame_leave();
    return 0;
}
