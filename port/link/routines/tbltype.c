/* Routines of units/tbltype.c reached by identifier: the row-draw callbacks
 * (0c45:0b0c) stored as far code pointers in the list windows (+0b) and
 * called by the list UI (LISTUI event 19, draw callback). Arguments in the
 * address-keyed convention: (column, first, row, last, flags), bytes
 * zero-extended, in prototype order. The host entry keeps its frame
 * buffers at the DS offset `scratch` (column outputs at +0..2, text buffers
 * from +4, at most 0xa8 bytes), placed in a modeled DGROUP frame. */
#include "../units/units_F.h"

PORT_ROUTINE(opeu_tbl_draw_types)
{
    uint16_t sp = port_stack_enter(LF_FRAME);
    opeu_tbl_draw_types(ds, link_tbltype, (uint16_t)(sp - 0xc0u), (uint8_t)ARG(0), (uint8_t)ARG(1),
        (uint8_t)ARG(2), (uint8_t)ARG(3), (uint8_t)ARG(4));
    port_stack_leave(LF_FRAME);
    return 0;
}
