/* Link: core/advtide.c (1dd2:19e2..1e43 "Tide of War"). Services 0
 * request_format, 1 control_message (c2_adv.h). */
#include "c2_adv.h"

uint16_t link_advtide(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    return c2_adv_service("advtide", ds, id, args, count);
}
