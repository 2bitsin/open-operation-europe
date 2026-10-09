/* Link: io/resopen.c (0977:000e open_resource, 0977:0038 disk_reset).
 * Events, from the host branch's comment (src/main/io/resopen.c):
 *   0 open(name offset, mode) -> handle   0000:001e opeu_dos_open
 *   1 close(handle)                       0000:0032 opeu_dos_close
 *   2 interrupt(0x21, AH): the only site issues AH=0Dh, disk reset. */
#include "link.h"

uint16_t link_resopen(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    switch (id) {
    case 0: return (uint16_t)opeu_dos_open((const char *)PORT_DS(ARG(0)), ARG(1));
    case 1: return (uint16_t)opeu_dos_close((int16_t)ARG(0));
    case 2:
        if (ARG(0) == 0x21 && ARG(1) == 0x0d) {
            port_disk_reset();
            return 0;
        }
        port_fatal("resopen: interrupt %02x AH=%02x is not issued by MAIN", ARG(0), ARG(1));
        break;
    default: port_unhandled("resopen", id, count);
    }
    (void)ds;
    return 0;
}
