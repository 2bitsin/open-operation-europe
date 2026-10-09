/* Link: units/tbltype.c (0c45:0b0c element type table). Services: the TableEvent protocol of
 * TBLGROUP (kinds 0..4, 6, 7 used here); the conversions are shared with the
 * other row-draw modules in tblunit.c (lf_tblunit_table_service). */
#include "units_F.h"

uint16_t lf_tblunit_table_service(uint8_t *ds, const char *module, uint16_t id, const uint16_t *args, uint16_t count);

uint16_t link_tbltype(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    return lf_tblunit_table_service(ds, "tbltype", id, args, count);
}
