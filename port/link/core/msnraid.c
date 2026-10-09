/* Link: core/msnraid.c (0e8e:028c..06f7 raid and sabotage missions).
 * Services 0, 1, 6, 7, 8 and, through the msnrept helpers it calls with
 * this event, the rest of the mission table (c2_msn.h). */
#include "c2_msn.h"

uint16_t link_msnraid(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    return c2_msn_service("msnraid", ds, id, args, count);
}
