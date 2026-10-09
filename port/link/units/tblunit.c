/* Link: units/tblunit.c (0c45:03a6 unit table, 0c45:0650 unit orders
 * table). Services: the TableEvent protocol of TBLGROUP (core/tblgroup.c
 * host comment), shared by the row-draw modules TBLUNIT, TBLTYPE, TBLELEM,
 * TBLSUPP, TBLSTR, RQARMTBL and RQSUPTBL:
 *   0 column layout (column,row,out0,out1,out2) 0a8e:0140 table_columns
 *   1 position (x,y)                            0480:0808 army_position
 *   2 sprintf (dest,format,args...)             0480:0b9a request_format
 *   3 text color (color)                        0480:082e text_color
 *   4 side color (side)                         0b73:04e2 select_side_color
 *   5 ui text (text)                            0480:0484 ui_text
 *   6 print (format,args...)                    0480:0bb8 cursor_print (army_print)
 *   7 put range (s,start,count)                 0480:0bd0 text_put_range
 *   8 request text (id) -> DS offset            312c:000c message_lookup
 * The recovered name helpers called with this module's event
 * (unit_name 0b73:0188, location_name 0b73:023c) raise kind 2 as well. */
#include "units_F.h"

uint16_t lf_tblunit_table_service(uint8_t *ds, const char *module, uint16_t id, const uint16_t *args, uint16_t count);

uint16_t lf_tblunit_table_service(uint8_t *ds, const char *module, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t r = 0;
    switch (id) {
    case 0: opeu_table_columns(ds, link_listui, (uint8_t)ARG(0), (uint8_t)ARG(1), ARG(2), ARG(3), ARG(4)); break;
    case 1: opeu_army_position(ds, link_textout_host(), (int16_t)ARG(0), (int16_t)ARG(1)); break;
    case 2: r = lf_request_format(ds, args, count); break;
    case 3: opeu_text_color(ds, link_textout_host(), ARG(0)); break;
    case 4: opeu_select_side_color(ds, link_sideutil, (uint8_t)ARG(0)); break;
    case 5: opeu_ui_text(ds, link_textout_host(), ARG(0)); break;
    case 6: r = lf_cursor_print(ds, args, count); break;
    case 7: opeu_text_put_range(ds, link_textout_host(), ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2)); break;
    case 8: r = opeu_message_lookup(ds, link_msglook, (int16_t)ARG(0)); break;
    default: port_unhandled(module, id, count);
    }
    return r;
}

uint16_t link_tblunit(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    return lf_tblunit_table_service(ds, "tblunit", id, args, count);
}
