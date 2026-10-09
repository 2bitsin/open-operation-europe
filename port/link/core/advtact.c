/* Link: core/advtact.c (1dd2:0004..0a9d "Tactical Advice"). Services 0
 * request_format, 1 control_message (c2_adv.h); unit_name's own event is
 * remapped to 0 inside the host branch. */
#include "c2_adv.h"

uint16_t link_advtact(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    return c2_adv_service("advtact", ds, id, args, count);
}
