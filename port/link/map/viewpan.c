/* Link: map/viewpan.c (17c4:0242 pan the tactical view). Events (host
 * branch comment): 0 = erase (x,y) 15c4:0f7c, 1 = scroll_to (x,y,side)
 * 15c4:135e, x/y as 16-bit words. */
#include "map_common.h"

uint16_t link_viewpan(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t frame;
    switch (id) {
    case 0:                                                         /* 15c4:0f7c */
        frame = map_frame_enter();
        opeu_map_erase(ds, link_maperase, frame, (uint8_t)ARG(0), (uint8_t)ARG(1));
        map_frame_leave();
        break;
    case 1:                                                         /* 15c4:135e */
        frame = map_frame_enter();
        opeu_map_scroll_to(ds, link_mapscrol, frame, (int16_t)ARG(0), (int16_t)ARG(1), (uint8_t)ARG(2));
        map_frame_leave();
        break;
    default: port_unhandled("viewpan", id, count);
    }
    return 0;
}
