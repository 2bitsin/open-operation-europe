/* Link: core/rqcommon.c (22f9:000e..0135 request-menu helpers).
 * Services raised (host branch comment): 10 control_message, 19
 * request_format, 20 confirm_message, 21 pending_finish, 22 daily_show;
 * the request-menu table is in c2_rq.h. */
#include "c2_rq.h"

uint16_t link_rqcommon(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    return c2_rq_service("rqcommon", ds, id, args, count);
}
