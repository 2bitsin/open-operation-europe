/* Link: core/airforce.c (23b2:106e..1230, Air Force command). Services: ids
 * 0 (also location_name's label format), 4, 8, 9, 12, 13 and 14 (far
 * mission action from DS:a62e: 23b2:05b0/0b28/0e20, PORT_ROUTINE handlers
 * in port/link/routines/air{bomb,drop,lift}.c) of the 23b2 table shared by
 * the air-force modules (c2_air.h). */
#include "c2_air.h"

uint16_t link_airforce(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    return c2_air_23b2(ds, "airforce", id, args, count);
}
