/* Link: core/rqmenu.c (22f9:0958..0b95 request menu, turn action 7).
 * Services raised (host branch comment): 3 request_pending, 10
 * control_message, 16 army_title, 17 delay_dialog, 26 army_position, 27
 * army_print, 28 request_mode_is, 29 side_has_city, 30 far handler from
 * DS:a01e (request_supplies 22f9:0136 / request_arms 22f9:03b6 /
 * request_regiment 22f9:0780 through the routine link table); the
 * request-menu table is in c2_rq.h. */
#include "c2_rq.h"

uint16_t link_rqmenu(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    return c2_rq_service("rqmenu", ds, id, args, count);
}
