/* Routines of core/textout.c reached by address from the battle modules and
 * stockord (address-keyed words, port.h; see map/btl_link.h):
 *   0480:00a4 input_flush ()               0480:0484 ui_text (BX s)
 *   0480:0808 army_position (AX x, DX y)   0480:082e text_color (AX color)
 *   0480:0838 text_paper_set (AL paper)    0480:0844 context_save ()
 *   0480:086a context_restore ()           0480:0b9a request_format (cdecl)
 *   0480:0bb8 cursor_print (cdecl)         0480:0bd0 text_put_range (AL start,
 *                                                     DL count, BX s) */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_input_flush)
{
    (void)args; (void)count;
    opeu_input_flush(ds, link_textout_host());
    return 0;
}

PORT_ROUTINE(opeu_ui_text)
{
    opeu_ui_text(ds, link_textout_host(), ARG(0));
    return 0;
}

PORT_ROUTINE(opeu_army_position)
{
    opeu_army_position(ds, link_textout_host(), (short)ARG(0), (short)ARG(1));
    return 0;
}

PORT_ROUTINE(opeu_text_color)
{
    opeu_text_color(ds, link_textout_host(), ARG(0));
    return 0;
}

PORT_ROUTINE(opeu_text_paper_set)
{
    opeu_text_paper_set(ds, link_textout_host(), (uint8_t)ARG(0));
    return 0;
}

PORT_ROUTINE(opeu_context_save)
{
    (void)args; (void)count;
    opeu_context_save(ds, link_textout_host());
    return 0;
}

PORT_ROUTINE(opeu_context_restore)
{
    (void)args; (void)count;
    opeu_context_restore(ds, link_textout_host());
    return 0;
}

/* sprintf(destination, format, ...): every word as pushed. */
PORT_ROUTINE(opeu_request_format)
{
    btl_frame_call(ds, opeu_request_format, args, count);
    return 0;
}

/* printf to the text cursor (army_print/context_print/turn_label). */
PORT_ROUTINE(opeu_cursor_print)
{
    btl_frame_call(ds, opeu_cursor_print, args, count);
    return 0;
}

PORT_ROUTINE(opeu_text_put_range)
{
    opeu_text_put_range(ds, link_textout_host(), ARG(2), (uint8_t)ARG(0), (uint8_t)ARG(1));
    return 0;
}
