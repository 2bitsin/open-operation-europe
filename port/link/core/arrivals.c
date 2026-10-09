/* Link: core/arrivals.c (1270:0ada process_arrivals, 1270:0c32
 * hourly_c_stage_c). Services (event(ds,id,a0..a3)): locate=0 (x/y DS
 * offsets, side), prepare=1 (unit), context=2 (value), location=3
 * (x,y,side), unit view=4 (unit), show=5 (id, side, far offset, segment),
 * finish=6 (value). */
#include "c1_common.h"

uint16_t link_arrivals(uint8_t *ds, uint16_t id, uint16_t a0, uint16_t a1, uint16_t a2, uint16_t a3)
{
    uint16_t sp, r = 0;
    switch (id) {
    case 0:                                                                      /* 1270:000e */
        r = opeu_arrival_locate(ds, link_arrplace, a0, a1, (uint8_t)a2);
        break;
    case 1:                                                                      /* 15c4:0fd0 */
        /* map frame convention: the callee's BP (entry SP - 2). */
        sp = port_stack_enter(PORT_FRAME);
        opeu_arrival_prepare(ds, link_maperase, (uint16_t)(sp - 2), a0);
        port_stack_leave(PORT_FRAME);
        break;
    case 2: opeu_set_turn_context(ds, link_sound, (uint8_t)a0); break;            /* 0d65:00fe */
    case 3:                                                                      /* 15c4:1408 */
        sp = port_stack_enter(PORT_FRAME);
        opeu_arrival_location(ds, link_mappos, (uint16_t)(sp - 2), (uint8_t)a0, (uint8_t)a1, (uint8_t)a2);
        port_stack_leave(PORT_FRAME);
        break;
    case 4:                                                                      /* 15c4:0808 */
        sp = port_stack_enter(PORT_FRAME);
        opeu_arrival_view(ds, link_mapunit, (uint16_t)(sp - 2), a0);
        port_stack_leave(PORT_FRAME);
        break;
    case 5:                                                                      /* 1707:03e4 */
        opeu_daily_show(ds, link_dialogs_host(), a0, (uint8_t)a1, (uint32_t)a3 << 16 | a2);
        break;
    case 6: opeu_finish_side_initialization(ds, link_sound, (uint8_t)a0); break;  /* 0d65:0032 */
    default: port_unhandled("arrivals", id, 4);
    }
    return r;
}
