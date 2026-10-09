/* Routines of core/msnraid.c reached by identifier: mission handlers of the
 * DS:7a54 far table (data/dgevents.c), called by msntick service 17 with
 * args[0] = side. The host entries carve their buffers below `frame`. */
#include "core/c2_common.h"

PORT_ROUTINE(opeu_mission_raid_objective)
{
    uint16_t frame = c2_frame_enter();
    opeu_mission_raid_objective(ds, link_msnraid, frame, (uint8_t)ARG(0));
    c2_frame_leave();
    return 0;
}

PORT_ROUTINE(opeu_mission_raid_unit)
{
    uint16_t frame = c2_frame_enter();
    opeu_mission_raid_unit(ds, link_msnraid, frame, (uint8_t)ARG(0));
    c2_frame_leave();
    return 0;
}

PORT_ROUTINE(opeu_mission_sabotage)
{
    uint16_t frame = c2_frame_enter();
    opeu_mission_sabotage(ds, link_msnraid, frame, (uint8_t)ARG(0));
    c2_frame_leave();
    return 0;
}

PORT_ROUTINE(opeu_mission_objective_3)
{
    uint16_t frame = c2_frame_enter();
    opeu_mission_objective_3(ds, link_msnraid, frame, (uint8_t)ARG(0));
    c2_frame_leave();
    return 0;
}

PORT_ROUTINE(opeu_mission_unit_4)
{
    uint16_t frame = c2_frame_enter();
    opeu_mission_unit_4(ds, link_msnraid, frame, (uint8_t)ARG(0));
    c2_frame_leave();
    return 0;
}
