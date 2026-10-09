/* Link: core/advregt.c (1dd2:0a9e..116d "Regiment Status"). Services 0
 * request_format, 1 control_message (c2_adv.h); unit_name's own event is
 * remapped to 0 inside the host branch. */
#include "c2_adv.h"

uint16_t link_advregt(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    return c2_adv_service("advregt", ds, id, args, count);
}
