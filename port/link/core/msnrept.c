/* Link: core/msnrept.c (0e8e:0000..028b special-mission report helpers).
 * Services 1..6 and 9..13 of the mission table (c2_msn.h). */
#include "c2_msn.h"

uint16_t link_msnrept(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    return c2_msn_service("msnrept", ds, id, args, count);
}
