/* Link: io/loadgame.c (06db:07c6..09c7 saved-game check and load menu).
 * Events, from the host branch's comment:
 *   0 exists(name) FILEIO opeu_dos_exists 0000:00c5
 *   1 open(name) 0977:000e opeu_open_resource
 *   2 seek(h,lo,hi,origin) 0000:009b   3 read(h,DS offset,count) 0000:0043
 *   4 close(h) 0000:0032
 *   5 print(format,value) 0480:0bb8 opeu_army_print (host opeu_cursor_print)
 *   6 attribute(value) 0480:0838 (host opeu_text_paper_set)
 *   7 framed text(x,y,text) 0a04:0788 (host opeu_title_box) -> DS offset
 *   8 UI text(text) 0480:0484   9 wait(tenths) 0a8e:004c opeu_delay_units
 *  10 menu(menu,first,count) 0a8e:0bc0 opeu_list_dialog
 *  11 backdrop(mode) 0a04:0720 opeu_side_backdrop (host opeu_backdrop_fill_color) */
#include "b1_util.h"

uint16_t link_loadgame(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t r = 0;
    switch (id) {
    case 0: r = (uint16_t)opeu_dos_exists((const char *)PORT_DS(ARG(0))); break;
    case 1: r = (uint16_t)opeu_open_resource(ds, link_resopen, ARG(0)); break;
    case 2: r = (uint16_t)opeu_dos_seek((int16_t)ARG(0), (int32_t)ARG32(1), (int16_t)ARG(3)); break;
    case 3: r = (uint16_t)opeu_dos_read((int16_t)ARG(0), PORT_DS(ARG(1)), ARG(2)); break;
    case 4: r = (uint16_t)opeu_dos_close((int16_t)ARG(0)); break;
    case 5: b1_cursor_print(ds, args, count); break;
    case 6: opeu_text_paper_set(ds, link_textout_host(), (uint8_t)ARG(0)); break;
    case 7: r = opeu_title_box(ds, link_uiprim_host(), ARG(0), ARG(1), ARG(2)); break;
    case 8: opeu_ui_text(ds, link_textout_host(), ARG(0)); break;
    case 9: opeu_delay_units(ds, link_waitkey, (uint8_t)ARG(0)); break;
    case 10: r = opeu_list_dialog(ds, link_listdlg, ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2)); break;
    case 11: opeu_backdrop_fill_color(ds, link_uiprim_host(), (uint8_t)ARG(0)); break;
    default: port_unhandled("loadgame", id, count);
    }
    return r;
}
