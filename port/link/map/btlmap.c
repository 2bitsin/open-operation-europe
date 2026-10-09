/* Link: map/btlmap.c (2a96, battle map set-up). Event ids from the host
 * branch and tests/validate_battle_map.py (SERVICES, segment:offset):
 *   0 backdrop_fill_color(c) 0a04:0720   1 sprite_draw (11 cdecl words) 0000:3f26
 *   2 open_resource(BX name) 0977:000e    3 dos_seek(handle, lo, hi, origin) 0000:009b
 *   4 dos_read(handle, buffer, n) 0000:0043   5 dos_close(handle) 0000:0032
 *   6 army_position(x, y) 0480:0808       7 context_frame(x, y, w) 0a04:02a0
 *   8 ui_text(s) 0480:0484                9 army_print(format, ...) 0480:0bb8
 *  10 find_objective_at(x, y) 145c:016a  11 vga_clear() 0000:2aac
 *  12 palette_title_screen() 0a04:0884   13 draw_cell_above(x, y) 2864:0b72
 *  14 draw_cell(x, y) 2864:0ab2
 *  15 byte of the far HEXDISP plane at (segment, offset) = (DS:e702, DS:e700)
 *     words: the original reads it through opeu_gfx_source. */
#include "map/btl_link.h"

uint16_t link_btlmap(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t r = 0;
    switch (id) {
    case 0: opeu_backdrop_fill_color(ds, link_uiprim_host(), (uint8_t)ARG(0)); break;
    case 1:
        /* x, y, w, h, image (off, seg), mask (off, seg), buffer (off, seg), flags */
        koei_3f26(ARG(0), ARG(1), ARG(2), ARG(3), port_far(ARG(5), ARG(4)), port_far(ARG(7), ARG(6)),
                  port_far(ARG(9), ARG(8)), ARG(10));
        break;
    case 2: r = (uint16_t)opeu_open_resource(ds, link_resopen, ARG(0)); break;
    case 3: r = (uint16_t)opeu_dos_seek((int16_t)ARG(0), (int32_t)ARG32(1), (int16_t)ARG(3)); break;
    case 4: r = (uint16_t)opeu_dos_read((int16_t)ARG(0), PORT_DS(ARG(1)), ARG(2)); break;
    case 5: r = (uint16_t)opeu_dos_close((int16_t)ARG(0)); break;
    case 6: opeu_army_position(ds, link_textout_host(), (short)ARG(0), (short)ARG(1)); break;
    case 7: opeu_context_frame(ds, link_uiprim_host(), ARG(0), ARG(1), ARG(2)); break;
    case 8: opeu_ui_text(ds, link_textout_host(), ARG(0)); break;
    case 9: btl_frame_call(ds, opeu_cursor_print, args, count); break;
    case 10: r = opeu_find_objective_at(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;
    case 11: r = (uint16_t)opeu_vga_clear(); break;
    case 12: opeu_palette_title_screen(ds, link_uiprim_host()); break;
    case 13: opeu_battle_draw_cell_above(ds, link_btlcell, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;
    case 14: opeu_battle_draw_cell(ds, link_btlcell, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;
    case 15: r = *port_far(ARG(0), ARG(1)); break;
    default: port_unhandled("btlmap", id, count);
    }
    return r;
}
