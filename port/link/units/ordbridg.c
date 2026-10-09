/* Link: units/ordbridg.c (3131:0596..06a9 destroy-bridge order handler).
 * Services of tests/validate_order_handlers.py used here: 0 unit_average11,
 * 1 rand_bounded, 2 restore_unit_order, 8 unit_report, 12 daily_show,
 * 15 move_adjacent, 16 move_level_lower, 17 cell_side, 24 unit_person,
 * 25 unit_average10, 27 unit_order_destination, 28 map_code,
 * 29 map_mark_5, 30 map_erase, 31 pending_finish, 33 sound_cue5
 * (see ordhand_F.h). */
#include "ordhand_F.h"

uint16_t link_ordbridg(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t r = 0;
    switch (id) {
    case 0: case 1: case 2: case 8: case 12: case 15: case 16: case 17:
    case 24: case 25: case 27: case 28: case 29: case 30: case 31: case 33:
        lf_order_handler_service(ds, id, args, count, &r);
        break;
    default: port_unhandled("ordbridg", id, count);
    }
    return r;
}
