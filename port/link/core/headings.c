/* Link: core/headings.c (0bc3:023a opeu_army_heading, 0bc3:02e6
 * opeu_division_heading). HeadingEvent ids, from the host branch's macros,
 * resolved by address (symbols.json):
 *   0 fill(x1,y1,x2,y2,color) 0000:2fa5 opeu_ui_fill
 *   1 outline(x1,y1,x2,y2,color) 0a04:065a opeu_ui_outline
 *   2 cursor(x,y) 0480:0808 opeu_context_position (host opeu_army_position)
 *   3 text(name) 0480:0484 opeu_ui_text
 *   5 print(format,...) 0480:0bb8 opeu_context_print (host opeu_cursor_print)
 *   6 portrait(x,y,person) 0a04:046c opeu_context_portrait
 *   7 line(x1,y1,x2,y2,color) 0000:2d16 opeu_ui_line
 * (id 4 is not raised: the side colour goes through the recovered
 * 0b73:04e2 opeu_select_side_color, which reports its colour on the
 * HeadingColorEvent: COLOR(c) = 0480:082e opeu_text_color(c).) */
#include "b1_util.h"

void link_headings_HeadingEvent(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    switch (id) {
    case 0: opeu_ui_fill(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4)); break;
    case 1: opeu_ui_outline(ds, link_uiprim_host(), ARG(0), ARG(1), ARG(2), ARG(3), ARG(4)); break;
    case 2: opeu_army_position(ds, link_textout_host(), (int16_t)ARG(0), (int16_t)ARG(1)); break;
    case 3: opeu_ui_text(ds, link_textout_host(), ARG(0)); break;
    case 5: b1_cursor_print(ds, args, count); break;
    case 6: opeu_context_portrait(ds, link_uiprim_host(), ARG(0), ARG(1), ARG(2)); break;
    case 7: opeu_ui_line(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4)); break;
    default: port_unhandled("headings", id, count);
    }
}

void link_headings_HeadingColorEvent(uint8_t *ds, uint16_t color)
{
    opeu_text_color(ds, link_textout_host(), color);
}
