/* Link: core/airshow.c (0f43:00a4..028a, air-operation display). Services,
 * from the host branch's event list (airshow.c host comment):
 *  0 bitmap_save(x,y,w,h,segment,offset)                       0000:2ed8
 *  1 draw_far(x,y,w,h,mode,segment,offset)                     0000:28b0
 *  2 sprite_compose(x,y,w,h,image off,image seg,mask off,mask seg,
 *                   mode,segment,offset)                       0000:3e0a
 *  3 bitmap_restore(x,y,w,h,mode,segment,offset)               0000:2674
 *  4 request_format(destination,format,colour,argument)        0480:0b9a
 *  5 control_message(text,person)                              1707:0672
 * Far (segment, offset) pairs become host pointers (port_far). */
#include "c2_air.h"

uint16_t link_airshow(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t r = 0;
    switch (id) {
    case 0:
        opeu_bitmap_save(ARG(0), ARG(1), ARG(2), ARG(3), port_far(ARG(4), ARG(5)));
        break;
    case 1:
        r = (uint16_t)opeu_draw_far(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), port_far(ARG(5), ARG(6)));
        break;
    case 2:
        opeu_sprite_compose(ARG(0), ARG(1), ARG(2), ARG(3), port_far(ARG(5), ARG(4)),
                            port_far(ARG(7), ARG(6)), port_far(ARG(9), ARG(10)), ARG(8));
        break;
    case 3:
        r = (uint16_t)opeu_bitmap_restore(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), port_far(ARG(5), ARG(6)));
        break;
    case 4: c2_request_format(ds, args, count); break;
    case 5: opeu_control_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break;
    default: port_unhandled("airshow", id, count);
    }
    return r;
}
