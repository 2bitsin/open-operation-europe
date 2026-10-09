/* Link: io/mouse.c (05a8:000c..0265 INT 33h mouse services).
 * The module's service is the shared 05a8 input boundary; MOUSE itself
 * issues only call 1 = int86(0x33) with seven REGS words (ax bx cx dx si di
 * cflag) in and out. The call sites issue AX = 0 (reset), 1 (show), 2 (hide),
 * 3 (position and buttons), 4 (set position, CX/DX), 7 (x range CX..DX) and
 * 8 (y range CX..DX); each goes to the matching native driver function.
 * Other modules sharing the boundary (KEYBOARD, MENUDRAW, NUMDLG) forward
 * their call 1 here. */
#include "link.h"

/* int86 copies the output registers back and returns AX. */
static uint16_t mouse_int33(uint16_t *w)
{
    switch (w[0]) {
    case 0:
        w[0] = port_mouse_reset();
        w[1] = 2;                       /* BX: number of buttons */
        break;
    case 1: port_mouse_show(); break;
    case 2: port_mouse_hide(); break;
    case 3: port_mouse_state(&w[1], &w[2], &w[3]); break;
    case 4: port_mouse_set_position(w[2], w[3]); break;
    case 7: port_mouse_x_range(w[2], w[3]); break;
    case 8: port_mouse_y_range(w[2], w[3]); break;
    default: port_fatal("mouse: INT 33h function %04x is not issued by MAIN", w[0]);
    }
    w[6] = 0;                           /* cflag: int86 copies CF, clear */
    return w[0];
}

uint16_t link_mouse(uint8_t *ds, uint16_t call, uint16_t *words)
{
    (void)ds;
    switch (call) {
    case 1: return mouse_int33(words);
    default: port_unhandled("mouse", call, 7);
    }
    return 0;
}
