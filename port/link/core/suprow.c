/* Link: core/suprow.c (20b3:0452 opeu_supply_draw_row). Services, from the
 * host branch (the SUPPANEL numbering; 11 is raised by opeu_supply_digits,
 * which the row redraw calls with this callback):
 *  9 army_position(x, y)        0480:0808
 * 10 army_print(format, value)  0480:0bb8 (cdecl)
 * 11 glyph(x, y, w, h, colour, near glyph)  VGALIB 2A81 opeu_vga_glyph
 * 12 fill(x, y, 3, 6, 0)        VGALIB 2AD2 opeu_vga_fill_rect = ui_box
 * 13 army_print(format)         0480:0bb8 (cdecl) */
#include "b2_common.h"

uint16_t link_suprow(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    switch (id) {
    case 9: opeu_army_position(ds, link_textout_host(), (int16_t)ARG(0), (int16_t)ARG(1)); break;
    case 10: case 13: b2_cursor_print(ds, args, count); break;
    case 11: opeu_vga_glyph(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), PORT_DS(ARG(5))); break;
    case 12: opeu_ui_box(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4)); break;
    default: port_unhandled("suprow", id, count);
    }
    return 0;
}
