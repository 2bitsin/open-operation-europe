/* Link: units/xferstep.c (31f4:05a8 transfer step, 31f4:08fe run). The
 * module uses the transfer-panel numbering (xfer_F.h, xferpnl.c): 15..20,
 * 22..26; the panel routines it calls (draw_row, input) raise 1..11, 14
 * through this module's event. */
#include "xfer_F.h"

uint16_t link_xferstep(uint8_t *ds, uint16_t id, uint16_t *args, uint16_t count)
{
    return lf_xfer_service(ds, "xferstep", id, args, count);
}
