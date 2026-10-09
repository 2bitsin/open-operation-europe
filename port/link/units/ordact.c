/* Link: units/ordact.c (1c23:09aa unit action of the map cursor). Services,
 * from the host branch's list (shared order-module numbering, units_F.h):
 * 0 request_format, 1 control_message, 3 arrival_location, 4 pick_location,
 * 5 control_panel, 9 speaker_panel, 11 army_position,
 * 12 prompt_value_cancel, 13 control_position, 14 message_lookup,
 * 15 delay_dialog, 16 delay_units, 17 daily_context; plus
 * 2 confirm_message, raised through this callback by
 * opeu_cancel_unit_orders (ordunit, CANCEL passes ordact's event). */
#include "units_F.h"

uint16_t link_ordact(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t r = 0;
    switch (id) {
    case 0: case 1: case 2: case 3: case 4: case 5: case 9: case 11:
    case 12: case 13: case 14: case 15: case 16: case 17:
        lf_order_service(ds, id, args, count, &r);
        break;
    default: port_unhandled("ordact", id, count);
    }
    return r;
}
