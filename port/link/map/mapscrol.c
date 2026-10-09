/* Link: map/mapscrol.c (15c4:1054..13d7 tactical view clear/redraw/scroll).
 * Its MapEvent uses the 15c4 segment's shared service kinds
 * (map_common.h, served by map_plan.c); this module raises 10 pattern_draw, 11 screen_shift, 14 status_mode; runs MAPVIEW and MAPFRAME with this event. */
#include "map_common.h"

uint16_t link_mapscrol(uint8_t *ds, uint16_t kind, const uint16_t *args, uint16_t count)
{
    return map_plan_service(ds, "mapscrol", kind, args, count);
}
