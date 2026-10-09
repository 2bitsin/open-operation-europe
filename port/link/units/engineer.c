/* Link: units/engineer.c (319e:0002..0566 engineering order completion).
 * Services, from the host branch's list (tests/validate_engineer_orders.py):
 *  0 unit_person 145c:01c4        1 rand_bounded 09f6:00d4
 *  2 unit_quantity(unit,kind,&n) 145c:0596  3 unit_average10 145c:052e
 *  4 unit_average11 145c:0562     5 unit_order_destination 145c:02ea
 *  6 cell_marked(x,y,side) 145c:0eb2  7 move_adjacent 32e6:0a92
 *  8 move_level_lower 32e6:0c3e   9 cell_side 145c:0e42
 * 10 unit_report 1707:07ec       11 mark_cell(x,y,side) 145c:0e76
 * 12 map_erase 15c4:0f7c         13 daily_show(id,side,off,seg) 1707:03e4
 * 14 restore_unit_order 145c:02b4 15 unmark_cell(x,y,side) 145c:0e94
 * 16 map_code 145c:0dc0          17 map_mark_10(x,y) 145c:0d8c
 * 18 move_objective_target 32e6:0ae8  19 move_unit_target 32e6:0b38
 * 20 pending_finish 0a8e:005c
 * Callees shared with the 3131 order handlers go through ordhand_F.h with
 * that numbering (same callee, same argument packing). */
#include "ordhand_F.h"

uint16_t link_engineer(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    /* engineer id -> order-handler id (ordhand_F.h); 0xff: handled below */
    static const uint8_t handler_id[21] = {
        24, 1, 0xff, 25, 0, 27, 0xff, 15, 16, 17, 8, 0xff, 30, 12, 2, 0xff, 28, 0xff, 14, 21, 31
    };
    uint16_t r = 0;
    switch (id) {
    case 2: r = opeu_unit_quantity(ds, ARG(0), (uint8_t)ARG(1), PORT_DS(ARG(2))); break;
    case 6: r = opeu_cell_marked(ds, (uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2)); break;
    case 11: opeu_mark_cell(ds, (uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2)); break;
    case 15: opeu_unmark_cell(ds, (uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2)); break;
    case 17: opeu_map_mark_10(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;
    default:
        if (id >= 21 || !lf_order_handler_service(ds, handler_id[id], args, count, &r))
            port_unhandled("engineer", id, count);
    }
    return r;
}
