/* Link: core/msnrecon.c (0e8e:0910..0a77 reconnaissance missions).
 * Services 0, 6 and, through the msnrept helpers it calls with this event,
 * the rest of the mission table (c2_msn.h). */
#include "c2_msn.h"

uint16_t link_msnrecon(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    return c2_msn_service("msnrecon", ds, id, args, count);
}
