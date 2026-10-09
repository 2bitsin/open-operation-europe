/* Link: open/core/oemsmap.c (OPEN 032a:000e..01f1, EMS for the sprite
 * buffer). Events: the EMSDRV routines of OPEN segment 031c, MAIN's
 * src/main/asm/emsdrv.asm linked unchanged into OPEN (src/open/dos/
 * layout.json, object_program main), so the native EMSDRV of MAIN
 * (port/services/ems.c: no expanded memory manager) serves them. Pointer
 * arguments are DS offsets of OPEN's DGROUP:
 *   0 present()  1 status()  2 page_count(&count)  3 free_pages(&total, &free)
 *   4 frame(&segment)  5 allocate(pages, &handle)  6 page_array(pages)
 *   7 map(handle, physical, logical)  8 release(handle)
 * With no EMM, detection fails at present() and the opening allocates its
 * sixth buffer in DOS memory, as on a PC without EMS.
 */
#include "port.h"
#include "services.h"
#include "open_process.h"

#define ARG(i) ((uint16_t)((i) < count ? args[(i)] : 0))

uint16_t link_oemsmap(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    switch (id) {
    case 0: return (uint16_t)opeu_ems_present();
    case 1: return (uint16_t)opeu_ems_status();
    case 2: return (uint16_t)opeu_ems_page_count((int16_t *)(ds + ARG(0)));
    case 3: return (uint16_t)opeu_ems_free_pages((int16_t *)(ds + ARG(0)), (int16_t *)(ds + ARG(1)));
    case 4: return (uint16_t)opeu_ems_frame((uint16_t *)(ds + ARG(0)));
    case 5: return (uint16_t)opeu_ems_allocate((int16_t)ARG(0), (int16_t *)(ds + ARG(1)));
    case 6: return (uint16_t)opeu_ems_page_array(ds + ARG(0));
    case 7: return (uint16_t)opeu_ems_map((int16_t)ARG(0), ARG(1), (int16_t)ARG(2));
    case 8: return (uint16_t)opeu_ems_release((int16_t)ARG(0));
    default: port_unhandled("oemsmap", id, count);
    }
    return 0;
}
