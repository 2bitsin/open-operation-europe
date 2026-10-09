/* Link: map/mapover.c (15c4:08aa..0bff overview unit and cell draw).
 * Its MapEvent uses the 15c4 segment's shared service kinds
 * (map_common.h, served by map_plan.c); this module raises 6, 7, 8, 9, 16. */
#include "map_common.h"

uint16_t link_mapover(uint8_t *ds, uint16_t kind, const uint16_t *args, uint16_t count)
{
    return map_plan_service(ds, "mapover", kind, args, count);
}
