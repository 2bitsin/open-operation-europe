/* Link: core/tblpers.c (0c45:092c general/personnel row-draw callback).
 * Services: the TableEvent protocol of TBLGROUP, ids 0..4, 6, 7 and
 * 8 request text = 312c:000c message_lookup (b2_table_service). */
#include "b2_common.h"

uint16_t link_tblpers(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    return b2_table_service(ds, "tblpers", id, args, count);
}
