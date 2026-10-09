/* Link: core/daily.c (0e36:005c scan, 0e36:010a turn_daily). Services:
 * prepare=0, side context=1 (side), activate=2 (side), show=3 (event id,
 * side, far callback offset, far callback segment). */
#include "link.h"

uint16_t link_daily(uint8_t *ds, uint16_t id, uint16_t a0, uint16_t a1, uint16_t a2, uint16_t a3)
{
    uint16_t sp;
    switch (id) {
    case 0: opeu_turn_prepare(ds, link_dialogs_host()); break;               /* 1707:00c8 */
    case 1: opeu_daily_context(ds, link_context, (uint8_t)a0); break;        /* 1389:000e */
    case 2:                                                                  /* 15c4:0ff8 */
        /* MAPERASE frame convention: the routine's BP (entry SP - 2). */
        sp = port_stack_enter(PORT_FRAME);
        opeu_daily_activate(ds, link_maperase, (uint16_t)(sp - 2), (uint8_t)a0);
        port_stack_leave(PORT_FRAME);
        break;
    case 3:                                                                  /* 1707:03e4 */
        opeu_daily_show(ds, link_dialogs_host(), a0, (uint8_t)a1, (uint32_t)a3 << 16 | a2);
        break;
    default: port_unhandled("daily", id, 4);
    }
    return 0;
}
