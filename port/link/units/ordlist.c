/* Link: units/ordlist.c (1c23:0e42 order unit rows, 1c23:0e86 turn action 1,
 * the Orders command). Services from the host branch's list: 1
 * control_message, 2 confirm_message, 5 control_panel, 6 group_show,
 * 7 list_dialog, 8 unit_order_menu; and the services opeu_group_set_target
 * (map/ordtgt.c, 1c23:0044) raises through the event it receives from this
 * module (ordlist.c:132): 0 request_format, 1, 2, 3 arrival_location,
 * 4 pick_location. One numbering for all (units_F.h). */
#include "units_F.h"

uint16_t link_ordlist(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t r = 0;
    switch (id) {
    case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8:
        lf_order_service(ds, id, args, count, &r);
        break;
    default: port_unhandled("ordlist", id, count);
    }
    return r;
}
