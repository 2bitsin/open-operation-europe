/* Routines of core/uiprim.c reached by address from the battle modules:
 *   0a04:02a0 context_frame (AX x, DX y, BX width)
 *   0a04:046c context_portrait (AX x, DX y, BX person)
 *   0a04:0602 face_draw/ui_box/type_symbol (AX x, DX y, BX mode, stack key)
 *   0a04:065a ui_outline (AX x1, DX y1, BX x2, stack y2, color)
 *   0a04:07f4 palette_for_hour () */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_context_frame)
{
    opeu_context_frame(ds, link_uiprim_host(), ARG(0), ARG(1), ARG(2));
    return 0;
}

PORT_ROUTINE(opeu_context_portrait)
{
    opeu_context_portrait(ds, link_uiprim_host(), ARG(0), ARG(1), ARG(2));
    return 0;
}

PORT_ROUTINE(opeu_face_draw)
{
    opeu_face_draw(ds, link_uiprim_host(), ARG(0), ARG(1), ARG(2), (uint8_t)ARG(3));
    return 0;
}

PORT_ROUTINE(opeu_ui_outline)
{
    opeu_ui_outline(ds, link_uiprim_host(), ARG(0), ARG(1), ARG(2), ARG(3), ARG(4));
    return 0;
}

PORT_ROUTINE(opeu_palette_for_hour)
{
    (void)args; (void)count;
    opeu_palette_for_hour(ds, link_uiprim_host());
    return 0;
}
