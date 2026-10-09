/* Link: io/scenres.c (06db:0416..07c5 scenario text, message pictures and
 * per-scenario resources). Events, from the host branch's comment:
 *   0 open(name)                 0977:000e opeu_open_resource
 *   1 seek(h,lo,hi,origin)       FILEIO opeu_dos_seek      0000:009b
 *   2 read(h,DS offset,count)    FILEIO opeu_dos_read      0000:0043
 *   3 far read(h,seg,off,count)  FILEIO opeu_dos_read_far  0000:0057
 *   4 close(h)                   FILEIO opeu_dos_close     0000:0032
 *   5 EMS map dialog 06ac:0056   6 display prepare 06ac:007e
 *   7 view prepare 06ac:0042     8 graphics wait 06ac:006a */
#include "b1_util.h"

uint16_t link_scenres(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t r = 0;
    switch (id) {
    case 0: r = (uint16_t)opeu_open_resource(ds, link_resopen, ARG(0)); break;
    case 1: r = (uint16_t)opeu_dos_seek((int16_t)ARG(0), (int32_t)ARG32(1), (int16_t)ARG(3)); break;
    case 2: r = (uint16_t)opeu_dos_read((int16_t)ARG(0), PORT_DS(ARG(1)), ARG(2)); break;
    case 3: r = (uint16_t)opeu_dos_read_far((int16_t)ARG(0), ARG(1), ARG(2), ARG(3)); break;
    case 4: r = (uint16_t)opeu_dos_close((int16_t)ARG(0)); break;
    case 5: opeu_ems_map_dialog(ds, link_emsmap); break;
    case 6: opeu_display_prepare(ds, link_emsmap); break;
    case 7: opeu_view_prepare(ds, link_emsmap); break;
    case 8: opeu_gfx_wait(ds, link_emsmap); break;
    default: port_unhandled("scenres", id, count);
    }
    return r;
}
