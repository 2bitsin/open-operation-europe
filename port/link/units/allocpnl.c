/* Link: units/allocpnl.c (20b3:1908..1b4d allocation type panel). Services
 * from the host branch's list: 0 vga_copy(x,y,w,h,tx,ty) VGALIB 301F,
 * 1 context_frame(x,y,w) 0a04:02a0, 2 army_print(format,...) 0480:0bb8,
 * 3 type_symbol(x,y,p,code) 0a04:0602 (opeu_face_draw), 4 army_position(x,y)
 * 0480:0808. */
#include "units_F.h"

uint16_t link_allocpnl(uint8_t *ds, uint16_t id, uint16_t *args, uint16_t count)
{
    uint16_t r = 0;
    switch (id) {
    case 0: opeu_vga_copy(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5)); break;
    case 1: opeu_context_frame(ds, link_uiprim_host(), ARG(0), ARG(1), ARG(2)); break;
    case 2: r = lf_cursor_print(ds, args, count); break;
    case 3: opeu_face_draw(ds, link_uiprim_host(), ARG(0), ARG(1), ARG(2), (uint8_t)ARG(3)); break;
    case 4: opeu_army_position(ds, link_textout_host(), (int16_t)ARG(0), (int16_t)ARG(1)); break;
    default: port_unhandled("allocpnl", id, count);
    }
    return r;
}
