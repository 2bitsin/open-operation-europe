/* Link: units/ordissue.c (1c23:03c0 unit order menu). Services, from the
 * host branch's list (shared order-module numbering, see units_F.h):
 * 0 request_format, 1 control_message, 3 arrival_location, 4 pick_location,
 * 9 speaker_panel, 10 unit_announce, 11 army_position,
 * 12 prompt_value_cancel, 13 control_position, 14 message_lookup,
 * 15 delay_dialog, 16 delay_units; and 2 confirm_message, which
 * opeu_cancel_unit_orders (ordunit, 1c23:01fa) raises through the event it
 * receives from this module (ordissue.c:216). */
#include "units_F.h"

uint16_t link_ordissue(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t r = 0;
    switch (id) {
    case 0: case 1: case 2: case 3: case 4: case 9: case 10: case 11:
    case 12: case 13: case 14: case 15: case 16:
        lf_order_service(ds, id, args, count, &r);
        break;
    default: port_unhandled("ordissue", id, count);
    }
    return r;
}
