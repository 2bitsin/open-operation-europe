/* Link: map/inspect.c (17c4:00bc highlight an inspected cell). Events
 * (host branch comment): view=0 (17c4:0006), overview=1 (17c4:0080), each
 * with byte x,y; both are the recovered HIGHLITE routines. */
#include "map_common.h"

uint16_t link_inspect(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t frame;
    switch (id) {
    case 0:                                                         /* 17c4:0006 */
        frame = map_frame_enter();
        opeu_map_highlight_view(ds, link_highlite, frame, (uint8_t)ARG(0), (uint8_t)ARG(1));
        map_frame_leave();
        break;
    case 1:                                                         /* 17c4:0080 */
        frame = map_frame_enter();
        opeu_map_highlight_overview(ds, link_highlite, frame, (uint8_t)ARG(0), (uint8_t)ARG(1));
        map_frame_leave();
        break;
    default: port_unhandled("inspect", id, count);
    }
    return 0;
}
