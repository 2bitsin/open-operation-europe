/* Link: core/control2.c (1389:09ec opeu_turn_control_other, non-menu turn
 * control). Services: location(x, y, side)=0, final service()=1. */
#include "b2_common.h"

uint16_t link_control2(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    switch (id) {
    case 0:                                                  /* 15c4:1408 arrival_location */
        B2_SP_CALL(PORT_FRAME, opeu_arrival_location(ds, link_mappos, (uint16_t)(sp - 2),
                                                     (uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2)));
        break;
    case 1: opeu_control_other_finish(ds); break;            /* 1ac2:15f4 */
    default: port_unhandled("control2", id, count);
    }
    return 0;
}
