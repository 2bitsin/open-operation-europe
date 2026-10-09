/* Link: map/btlaimov.c, address-keyed battle AI group (path_step by value, see btl_ai_event in map/btl_link.h). */
#include "map/btl_link.h"

uint16_t link_btlaimov(uint8_t *ds, uint32_t service, const uint16_t *args, unsigned int count)
{
    return btl_ai_event(ds, service, args, count);
}
