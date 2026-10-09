/* Link: map/mappick.c (17c4:0274 interactive map location picker).
 * Events (host branch comment; callees from the DOS branch and
 * tests/validate_map_pick.py SERVICES):
 *   0 arrival location(x,y,side)   15c4:1408 opeu_arrival_location (MAPPOS)
 *   1 input reset                  05a8:01b4 opeu_input_reset (MOUSE)
 *   2 bitmap save(x,y,w,h,seg,off) 0000:2ed8 opeu_bitmap_save
 *   3 map erase(x,y)               15c4:0f7c opeu_map_erase (MAPERASE)
 *   4 inspect(x,y)                 17c4:00bc opeu_map_inspect (INSPECT)
 *   5 cursor(x,y)                  17c4:0164 opeu_map_cursor (CURSOR)
 *   9 mouse show                   05a8:003e opeu_mouse_show
 *  10 mouse buttons -> low byte    05a8:0170 opeu_mouse_buttons
 *  11 mouse position(x off,y off)  05a8:0086 opeu_mouse_position (writes
 *                                  the two DS words)
 *  12 mouse hide                   05a8:0062 opeu_mouse_hide
 *  13 viewport pan(dx,dy,x,side,y) 17c4:0242 opeu_viewport_pan: AX, DX, BX
 *                                  then the stack words side, y (the
 *                                  validator's packing); the routine is
 *                                  (dx,dy,x,y,side)
 *  17 keyboard key -> low byte     0541:00ba opeu_keyboard_key (KEYBOARD)
 *  18 bitmap restore(x,y,w,h,mode,seg,off) 0000:2674 opeu_bitmap_restore
 * VGALIB 2ed8 reads the destination segment at [bp+0e] and the offset at
 * [bp+10]; 2674 the segment at [bp+10] and the offset at [bp+12]. */
#include "map_common.h"

uint16_t link_mappick(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t frame, r = 0;
    switch (id) {
    case 0:                                                         /* 15c4:1408 */
        frame = map_frame_enter();
        opeu_arrival_location(ds, link_mappos, frame, (uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2));
        map_frame_leave();
        break;
    case 1: opeu_input_reset(ds, link_mouse); break;                /* 05a8:01b4 */
    case 2:                                                         /* 0000:2ed8 */
        opeu_bitmap_save(ARG(0), ARG(1), ARG(2), ARG(3), port_far(ARG(4), ARG(5)));
        break;
    case 3:                                                         /* 15c4:0f7c */
        frame = map_frame_enter();
        opeu_map_erase(ds, link_maperase, frame, (uint8_t)ARG(0), (uint8_t)ARG(1));
        map_frame_leave();
        break;
    case 4: opeu_map_inspect(ds, link_inspect, (uint8_t)ARG(0), (uint8_t)ARG(1)); break; /* 17c4:00bc */
    case 5:                                                         /* 17c4:0164 */
        frame = map_frame_enter();
        opeu_map_cursor(ds, link_cursor, frame, ARG(0), ARG(1));
        map_frame_leave();
        break;
    case 9: opeu_mouse_show(ds, link_mouse); break;                 /* 05a8:003e */
    case 10: r = opeu_mouse_buttons(ds, link_mouse); break;         /* 05a8:0170 */
    case 11: opeu_mouse_position(ds, link_mouse, ARG(0), ARG(1)); break; /* 05a8:0086 */
    case 12: opeu_mouse_hide(ds, link_mouse); break;                /* 05a8:0062 */
    case 13:                                                        /* 17c4:0242 */
        opeu_viewport_pan(ds, link_viewpan, (int16_t)ARG(0), (int16_t)ARG(1),
                          (uint8_t)ARG(2), (uint8_t)ARG(4), (uint8_t)ARG(3));
        break;
    case 17: r = opeu_keyboard_key(ds, link_keyboard); break;       /* 0541:00ba */
    case 18:                                                        /* 0000:2674 */
        opeu_bitmap_restore(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), port_far(ARG(5), ARG(6)));
        break;
    default: port_unhandled("mappick", id, count);
    }
    return r;
}
