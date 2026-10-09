/* Link: map/maperase.c (15c4:0f7c..1053 cell redraw, place/remove, overview activation).
 * Its MapEvent uses the 15c4 segment's shared service kinds
 * (map_common.h, served by map_plan.c); this module raises 14 status_mode; runs MAPOVER, MAPVIEW and MAPFRAME with this event. */
#include "map_common.h"

uint16_t link_maperase(uint8_t *ds, uint16_t kind, const uint16_t *args, uint16_t count)
{
    return map_plan_service(ds, "maperase", kind, args, count);
}
