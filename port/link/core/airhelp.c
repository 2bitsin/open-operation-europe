/* Link: core/airhelp.c (23b2:0006..0169, air-force dialog helpers).
 * Services: ids 0..7 of the 23b2 table shared by the air-force modules
 * (c2_air.h). The helpers are called directly by airbomb/airdrop/airlift
 * with those modules' own callbacks. */
#include "c2_air.h"

uint16_t link_airhelp(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    return c2_air_23b2(ds, "airhelp", id, args, count);
}
