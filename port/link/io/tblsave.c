/* Link: io/tblsave.c (0c45:0ff0..1201 saved-game list table callbacks).
 * Events, from the host branch's comment (src/main/io/tblsave.c "Host
 * model: see TBLGROUP"), resolved by address:
 *   0 column layout(column,row,out0,out1,out2) 0a8e:0140 opeu_table_columns
 *     (writes the three DS bytes)
 *   1 position(x,y) 0480:0808 opeu_army_position
 *   2 sprintf(destination,format,args...) 0480:0b9a opeu_request_format
 *   6 print(format,args...) 0480:0bb8 opeu_cursor_print
 *   7 put range(s,start,count) 0480:0bd0 opeu_text_put_range
 * Ids 3..5 belong to TBLGROUP's numbering and are not raised here.
 * The routines themselves are reached by identifier (DGLABELS window at
 * DS:5aa6 -> LISTUI events 19/20): port/link/routines/tblsave.c. */
#include "b1_util.h"

uint16_t link_tblsave(uint8_t *ds, uint16_t kind, const uint16_t *args, uint16_t count)
{
    switch (kind) {
    case 0:
        opeu_table_columns(ds, link_listui, (uint8_t)ARG(0), (uint8_t)ARG(1), ARG(2), ARG(3), ARG(4));
        break;
    case 1: opeu_army_position(ds, link_textout_host(), (int16_t)ARG(0), (int16_t)ARG(1)); break;
    case 2: b1_request_format(ds, args, count); break;
    case 6: b1_cursor_print(ds, args, count); break;
    case 7: opeu_text_put_range(ds, link_textout_host(), ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2)); break;
    default: port_unhandled("tblsave", kind, count);
    }
    return 0;
}
