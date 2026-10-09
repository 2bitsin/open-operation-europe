/* Link: core/advmenu.c (1dd2:2088..20fc staff advice menus). Services
 * (host branch comment): 1 control_message(text,person) 1707:0672,
 * 2 delay_dialog(text,last,person,menu) 1707:0726 -> choice, 3 far call
 * (offset,segment) of the DS:9060 advice / DS:90e2 staff action tables
 * through the routine link table, AX returned. */
#include "c2_adv.h"

uint16_t link_advmenu(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t r = 0;
    switch (id) {
    case 1: opeu_control_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break; /* 1707:0672 */
    case 2:                                                           /* 1707:0726 */
        r = opeu_delay_dialog(ds, link_dialogs_host(), ARG(0), ARG(1), ARG(2), ARG(3));
        break;
    case 3: r = (uint16_t)port_call(ds, (uint32_t)ARG(1) * 16u + ARG(0), 0, 0); break;
    default: port_unhandled("advmenu", id, count);
    }
    return r;
}
