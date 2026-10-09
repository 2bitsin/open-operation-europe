/* Link: units/ordarriv.c (32ca:0002..0125 arrive/reach/hold order handlers).
 * Services, from the host branch's list: 0 unit_order_destination(unit,
 * &x,&y) 145c:02ea, 1 unit_message(unit,text) 1707:07ec (unit_report),
 * 2 restore_unit_order(unit) 145c:02b4, 3 move_unit_target(&x,&y)
 * 32e6:0b38, 4 move_adjacent(x,y) 32e6:0a92, 5 unit_person(unit)
 * 145c:01c4, 6 move_level_lower(slot,value) 32e6:0c3e, 7 rand_bounded(15)
 * 09f6:00d4. */
#include "ordhand_F.h"

uint16_t link_ordarriv(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    /* Same callees as the 3131 order-handler numbering (ordhand_F.h). */
    static const uint8_t handler_id[8] = { 27, 8, 2, 21, 15, 24, 16, 1 };
    uint16_t r = 0;
    if (id >= 8 || !lf_order_handler_service(ds, handler_id[id], args, count, &r))
        port_unhandled("ordarriv", id, count);
    return r;
}
