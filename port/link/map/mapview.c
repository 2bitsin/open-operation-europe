/* Link: map/mapview.c (15c4:0c00..0f7b tactical unit sprite and cell redraw).
 * Its MapEvent uses the 15c4 segment's shared service kinds
 * (map_common.h, served by map_plan.c); this module raises 6, 8, 15, 16; runs MAPCELL (0..3) and MAPFRAME (4, 5) with this event. */
#include "map_common.h"

uint16_t link_mapview(uint8_t *ds, uint16_t kind, const uint16_t *args, uint16_t count)
{
    return map_plan_service(ds, "mapview", kind, args, count);
}
