/* Link: map/mapframe.c (15c4:012a..0549 view numbers, scale labels, rulers).
 * Its MapEvent uses the 15c4 segment's shared service kinds
 * (map_common.h, served by map_plan.c); this module raises 4 glyph_draw_near, 5 ui_line. */
#include "map_common.h"

uint16_t link_mapframe(uint8_t *ds, uint16_t kind, const uint16_t *args, uint16_t count)
{
    return map_plan_service(ds, "mapframe", kind, args, count);
}
