/* Link: core/vwcity.c (1fe6:0760..08cf city lists and menu).
 * Services: the shared ViewEvent protocol of the 1fe6 view modules
 * (b2_view_service in b2_view.h; ids as the module's host enum). */
#include "b2_view.h"

uint16_t link_vwcity(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    return b2_view_service(ds, "vwcity", id, args, count);
}
