/* Link: core/advhints.c (1dd2:1e44..2087 "Operation Hints"). Services 0
 * request_format, 1 control_message (c2_adv.h). */
#include "c2_adv.h"

uint16_t link_advhints(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    return c2_adv_service("advhints", ds, id, args, count);
}
