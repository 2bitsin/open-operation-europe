/* Routines of io/tblsave.c reached by identifier: the saved-game list
 * window (DGLABELS, DS:5aa6) stores them as far code pointers; LISTUI calls
 * them through events 19 (row draw) and 20 (select).
 *   0c45:0ff0 opeu_tbl_save_accept: select callback (AL = row) -> AL.
 *   0c45:0ff4 opeu_tbl_draw_saves: row draw (column, first, row, last,
 *     flags), bytes zero-extended, in prototype order. The host entry keeps
 *     its frame objects at `scratch` (column outputs +0..2, the 100-byte
 *     text buffer at +4), placed on the modeled DGROUP stack. */
#include "b1_util.h"

#define TBLSAVE_SCRATCH 0x68u

PORT_ROUTINE(opeu_tbl_save_accept)
{
    (void)ds; (void)args; (void)count;
    return opeu_tbl_save_accept();
}

PORT_ROUTINE(opeu_tbl_draw_saves)
{
    uint16_t scratch = b1_stack_alloc(TBLSAVE_SCRATCH);
    opeu_tbl_draw_saves(ds, link_tblsave, scratch, (uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2),
        (uint8_t)ARG(3), (uint8_t)ARG(4));
    b1_stack_free(TBLSAVE_SCRATCH);
    return 0;
}
