/* Link: core/msnkill.c (0e8e:06f8 assassination mission). Services 0, 1,
 * 3, 4, 9, 10 and, through the msnrept helpers it calls with this event,
 * the rest of the mission table (c2_msn.h). */
#include "c2_msn.h"

uint16_t link_msnkill(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    return c2_msn_service("msnkill", ds, id, args, count);
}
