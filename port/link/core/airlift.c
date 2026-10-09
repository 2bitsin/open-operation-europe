/* Link: core/airlift.c (23b2:0c7a..106d, airlift command). Services: ids
 * 0,1,2,3,10 of the 23b2 table shared by the air-force modules
 * (c2_air.h). */
#include "c2_air.h"

uint16_t link_airlift(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    return c2_air_23b2(ds, "airlift", id, args, count);
}
