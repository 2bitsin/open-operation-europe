/* Link: core/vwpeople.c (1fe6:0626..075f general lists and menu).
 * Services: the shared ViewEvent protocol of the 1fe6 view modules
 * (b2_view_service in b2_view.h; ids as the module's host enum). */
#include "b2_view.h"

uint16_t link_vwpeople(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    return b2_view_service(ds, "vwpeople", id, args, count);
}
