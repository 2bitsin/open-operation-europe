/* Link: map/mappos.c (15c4:13d8..1431 control position, centre, arrival location).
 * Its MapEvent uses the 15c4 segment's shared service kinds
 * (map_common.h, served by map_plan.c); this module raises nothing itself; runs MAPSCROL and MAPERASE with this event. */
#include "map_common.h"

uint16_t link_mappos(uint8_t *ds, uint16_t kind, const uint16_t *args, uint16_t count)
{
    return map_plan_service(ds, "mappos", kind, args, count);
}
