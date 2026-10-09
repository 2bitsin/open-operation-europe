/* Link: units/xferunit.c (31f4:0982..0d61 transfer table load/store and the
 * supply-unit / supply-objective order handlers). The module uses the
 * transfer-panel numbering (xfer_F.h, xferpnl.c): 19, 27..40; the panel
 * and step routines it calls (draw, run) raise the others through this
 * module's event. */
#include "xfer_F.h"

uint16_t link_xferunit(uint8_t *ds, uint16_t id, uint16_t *args, uint16_t count)
{
    return lf_xfer_service(ds, "xferunit", id, args, count);
}
