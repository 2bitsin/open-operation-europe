/* Link: core/advenemy.c (1dd2:116e..19e1 "Enemy Conditions"). Services 0
 * request_format, 1 control_message (c2_adv.h); unit_name's own event 2 is
 * remapped to 0 inside the host branch. */
#include "c2_adv.h"

uint16_t link_advenemy(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    return c2_adv_service("advenemy", ds, id, args, count);
}
