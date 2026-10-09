/* Link: units/finalact.c (32dc:0006 final turn action). Services, from the
 * host branch's list: 0 unit_order_destination(unit,&x,&y) 145c:02ea,
 * 6 move_level_lower(slot,0) 32e6:0c3e, 8 clear_unit_orders(unit,3)
 * 145c:020a, 9 order handler (offset, segment words of the DGBATTLE
 * far-code table DS:c0c8; the routine link table runs it), 10
 * move_level_raise(slot,value) 32e6:0c00, 11 unit_average9(unit) 145c:04fa,
 * 12 move_unit(repeat) 32e6:0732. */
#include "ordhand_F.h"

uint16_t link_finalact(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t r = 0;
    switch (id) {
    case 0: lf_order_handler_service(ds, 27, args, count, &r); break;
    case 6: lf_order_handler_service(ds, 16, args, count, &r); break;
    case 8: opeu_clear_unit_orders(ds, ARG(0), (uint8_t)ARG(1)); break;
    case 9: r = (uint16_t)port_call(ds, (uint32_t)ARG(1) * 16u + ARG(0), 0, 0); break;
    case 10: r = opeu_move_level_raise(ds, ARG(0), (uint8_t)ARG(1)); break;
    case 11: lf_order_handler_service(ds, 26, args, count, &r); break;
    case 12: lf_order_handler_service(ds, 19, args, count, &r); break;
    default: port_unhandled("finalact", id, count);
    }
    return r;
}
