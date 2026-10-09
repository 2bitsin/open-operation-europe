/* Link: core/rqsupply.c (22f9:0136 supplies request).
 * Services raised (host branch comment): 3 request_pending, 4
 * cancel_prompt, 6 request_file(target, argument, kind), 10
 * control_message, 16 army_title, 19 request_format, 20 confirm_message,
 * 23 map_pick_location; the request-menu table is in c2_rq.h. */
#include "c2_rq.h"

uint16_t link_rqsupply(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    return c2_rq_service("rqsupply", ds, id, args, count);
}
