/* Routines of core/tblgroup.c and core/tblpers.c reached by identifier:
 * the row-draw callbacks 0c45:0002 tbl_draw_groups, 0c45:01e6
 * tbl_draw_missions and 0c45:092c tbl_draw_staff, stored as far code
 * pointers in the DGLABELS table descriptors and called by the list UI
 * (LISTUI event 19): (column, first, row, last, flags), bytes
 * zero-extended, in prototype order. */
#include "core/b2_common.h"

PORT_ROUTINE(opeu_tbl_draw_groups)
{
    B2_SP_CALL(PORT_FRAME, opeu_tbl_draw_groups(ds, link_tblgroup, B2_TABLE_SCRATCH(sp), (uint8_t)ARG(0),
                                                (uint8_t)ARG(1), (uint8_t)ARG(2), (uint8_t)ARG(3), (uint8_t)ARG(4)));
    return 0;
}

PORT_ROUTINE(opeu_tbl_draw_missions)
{
    B2_SP_CALL(PORT_FRAME, opeu_tbl_draw_missions(ds, link_tblgroup, B2_TABLE_SCRATCH(sp), (uint8_t)ARG(0),
                                                  (uint8_t)ARG(1), (uint8_t)ARG(2), (uint8_t)ARG(3), (uint8_t)ARG(4)));
    return 0;
}

PORT_ROUTINE(opeu_tbl_draw_staff)
{
    B2_SP_CALL(PORT_FRAME, opeu_tbl_draw_staff(ds, link_tblpers, B2_TABLE_SCRATCH(sp), (uint8_t)ARG(0),
                                               (uint8_t)ARG(1), (uint8_t)ARG(2), (uint8_t)ARG(3), (uint8_t)ARG(4)));
    return 0;
}
