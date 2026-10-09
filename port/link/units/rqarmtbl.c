/* Link: units/rqarmtbl.c (22f9:04d2 regiment request table). Services: the TableEvent protocol of
 * TBLGROUP (kinds 0..4, 6, 7 (and 2 raised by unit_name 0b73:0188) used here); the conversions are shared with the
 * other row-draw modules in tblunit.c (lf_tblunit_table_service). */
#include "units_F.h"

uint16_t lf_tblunit_table_service(uint8_t *ds, const char *module, uint16_t id, const uint16_t *args, uint16_t count);

uint16_t link_rqarmtbl(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    return lf_tblunit_table_service(ds, "rqarmtbl", id, args, count);
}
