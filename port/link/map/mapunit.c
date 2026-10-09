/* Link: map/mapunit.c (15c4:060e..08a9 unit/group/arrival flashes and position reports).
 * Its MapEvent uses the 15c4 segment's shared service kinds
 * (map_common.h, served by map_plan.c); this module raises 1, 2, 3, 12 wait_ticks, 13 control_message; runs MAPCELL, MAPPOS, MAPERASE with this event. */
#include "map_common.h"

uint16_t link_mapunit(uint8_t *ds, uint16_t kind, const uint16_t *args, uint16_t count)
{
    return map_plan_service(ds, "mapunit", kind, args, count);
}
