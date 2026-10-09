/* Link: map/highlite.c (17c4:0006 tactical cell frame, 17c4:0080 overview
 * cursor glyph). Events (host branch comment): draw_far=0
 * (x,y,w,h,mode,seg,off) 0000:28b0, draw_near=1 (x,y,w,h,mode,ptr)
 * 0000:28a8 -- the same callees and words as the 15c4 kinds 0 and 1. */
#include "map_common.h"

uint16_t link_highlite(uint8_t *ds, uint16_t kind, const uint16_t *args, uint16_t count)
{
    switch (kind) {
    case 0: case 1: return map_plan_service(ds, "highlite", kind, args, count);
    default: port_unhandled("highlite", kind, count);
    }
    return 0;
}
