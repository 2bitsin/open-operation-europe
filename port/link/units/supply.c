/* Link: units/supply.c (20b3:08d0..0a4d supply exchange). Services from the
 * host branch's list: 100 supply_draw(first,second) 20b3:0332,
 * 101 supply_step() 20b3:05b8 -> done; and 2 request_format(buffer,format,
 * name,number) 0480:0b9a, which opeu_unit_name (unitname, 0b73:0188) raises
 * through the event it receives from this module (supply.c NAME). */
#include "units_F.h"

uint16_t link_supply(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t sp, r = 0;
    switch (id) {
    case 2: r = lf_request_format(ds, args, count); break;
    case 100: opeu_supply_draw(ds, link_suppanel, ARG(0), ARG(1)); break;
    case 101:
        /* selection: the selected-row byte of supply_step's frame (BP-8) */
        sp = port_stack_enter(LF_FRAME);
        r = opeu_supply_step(ds, link_supstep, (uint16_t)(sp - 2u - 8u));
        port_stack_leave(LF_FRAME);
        break;
    default: port_unhandled("supply", id, count);
    }
    return r;
}
