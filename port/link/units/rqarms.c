/* Link: units/rqarms.c (22f9:03b6 battalion request, 22f9:04ce row
 * predicate). Services from the host branch's list (request-menu
 * numbering, rq_F.h): 3 request_pending, 4 cancel_prompt, 6 request_file,
 * 9 control_panel, 10 control_message, 11 list_dialog, 19 request_format,
 * 20 confirm_message, 23 map_pick_location, 25 available_element_types. */
#include "rq_F.h"

uint16_t link_rqarms(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    switch (id) {
    case 3: case 4: case 6: case 9: case 10: case 11: case 19: case 20: case 23: case 25:
        return lf_rq_service("rqarms", ds, id, args, count);
    default: port_unhandled("rqarms", id, count);
    }
    return 0;
}
