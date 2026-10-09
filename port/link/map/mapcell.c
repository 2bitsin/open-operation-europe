/* Link: map/mapcell.c (15c4:0008..0129 cell paint, cell flash).
 * Its MapEvent uses the 15c4 segment's shared service kinds
 * (map_common.h, served by map_plan.c); this module raises 0 draw_far, 1 draw_near, 2 bitmap_restore_near, 3 bitmap_save_near. */
#include "map_common.h"

uint16_t link_mapcell(uint8_t *ds, uint16_t kind, const uint16_t *args, uint16_t count)
{
    return map_plan_service(ds, "mapcell", kind, args, count);
}
