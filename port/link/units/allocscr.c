/* Link: units/allocscr.c (20b3:15b8..1907 allocation screen). Services from
 * the host branch's list: 0 army_position(x,y) 0480:0808, 1 army_print
 * (format,...) 0480:0bb8, 2 vga_fill_rect(x,y,w,h,colour) VGALIB 2AD2,
 * 3 ui_outline(x1,y1,x2,y2,colour) 0a04:065a, 4 side_backdrop(mode)
 * 0a04:0720 (opeu_backdrop_fill_color). */
#include "units_F.h"

uint16_t link_allocscr(uint8_t *ds, uint16_t id, uint16_t *args, uint16_t count)
{
    uint16_t r = 0;
    switch (id) {
    case 0: opeu_army_position(ds, link_textout_host(), (int16_t)ARG(0), (int16_t)ARG(1)); break;
    case 1: r = lf_cursor_print(ds, args, count); break;
    case 2: r = (uint16_t)opeu_vga_fill_rect(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4)); break;
    case 3: opeu_ui_outline(ds, link_uiprim_host(), ARG(0), ARG(1), ARG(2), ARG(3), ARG(4)); break;
    case 4: opeu_backdrop_fill_color(ds, link_uiprim_host(), (uint8_t)ARG(0)); break;
    default: port_unhandled("allocscr", id, count);
    }
    return r;
}
