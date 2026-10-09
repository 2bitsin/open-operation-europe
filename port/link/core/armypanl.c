/* Link: core/armypanl.c (0bc3:037c..078f division regiment panel).
 * Services, from the host branch (source-order words):
 *  0 fill(x1,y1,x2,y2,colour)      VGALIB 2FA5 opeu_ui_fill
 *  1 outline(x1,y1,x2,y2,colour)   0a04:065a opeu_ui_outline
 *  2 line(x1,y1,x2,y2,colour)      VGALIB 2D16 opeu_ui_line
 *  3 position(x,y)                 0480:0808 army_position
 *  4 print(format,...)             0480:0bb8 army_print (cdecl)
 *  5 draw_far(x,y,w,h,mode,seg,off) VGALIB 28B0 opeu_draw_far
 *  6 element_name(element,buffer) -> AX  0b73:0234
 *  7 selection_eligible(unit) -> AX      145c:0fc4 */
#include "b2_common.h"

uint16_t link_armypanl(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t r = 0;
    switch (id) {
    case 0: opeu_ui_fill(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4)); break;
    case 1: opeu_ui_outline(ds, link_uiprim_host(), ARG(0), ARG(1), ARG(2), ARG(3), ARG(4)); break;
    case 2: opeu_ui_line(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4)); break;
    case 3: opeu_army_position(ds, link_textout_host(), (int16_t)ARG(0), (int16_t)ARG(1)); break;
    case 4: b2_cursor_print(ds, args, count); break;
    case 5: opeu_draw_far(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), B2_FAR(ARG(5), ARG(6))); break;
    case 6: r = opeu_element_name(ds, ARG(0), ARG(1)); break;
    case 7: r = opeu_unit_selection_eligible(ds, ARG(0)); break;
    default: port_unhandled("armypanl", id, count);
    }
    return r;
}
