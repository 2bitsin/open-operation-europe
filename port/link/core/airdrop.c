/* Link: core/airdrop.c (23b2:06ca..0c79, paratroop command). Services: ids
 * 0,1,2,3,10,11 of the 23b2 table shared by the air-force modules
 * (c2_air.h). The list-dialog callbacks it stores as far pointers
 * (23b2:06ca/06de/077a) are PORT_ROUTINE handlers in
 * port/link/routines/airdrop.c. */
#include "c2_air.h"

uint16_t link_airdrop(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    return c2_air_23b2(ds, "airdrop", id, args, count);
}
