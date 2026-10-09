/* Routines of io/mouse.c reached by address from the battle modules:
 *   0541:06ae mouse_show ()     0541:06d2 mouse_hide ()
 *   0541:06f6 mouse_position (AX y pointer, BX x pointer): writes both words
 *   0541:0774 mouse_left () -> AX   0541:079e mouse_right () -> AX
 *   0541:07e0 mouse_buttons () -> AL   0541:0824 input_reset () */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_mouse_show)
{
    (void)args; (void)count;
    opeu_mouse_show(ds, link_mouse);
    return 0;
}

PORT_ROUTINE(opeu_mouse_hide)
{
    (void)args; (void)count;
    opeu_mouse_hide(ds, link_mouse);
    return 0;
}

PORT_ROUTINE(opeu_mouse_position)
{
    opeu_mouse_position(ds, link_mouse, ARG(1), ARG(0));
    return 0;
}

PORT_ROUTINE(opeu_mouse_left)
{
    (void)args; (void)count;
    return (uint16_t)opeu_mouse_left(ds, link_mouse);
}

PORT_ROUTINE(opeu_mouse_right)
{
    (void)args; (void)count;
    return (uint16_t)opeu_mouse_right(ds, link_mouse);
}

PORT_ROUTINE(opeu_mouse_buttons)
{
    (void)args; (void)count;
    return opeu_mouse_buttons(ds, link_mouse);
}

PORT_ROUTINE(opeu_input_reset)
{
    (void)args; (void)count;
    opeu_input_reset(ds, link_mouse);
    return 0;
}
