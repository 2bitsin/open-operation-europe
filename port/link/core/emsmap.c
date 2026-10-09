/* Link: core/emsmap.c (06ac:0006..02f0 EMS page mapping). Events, from the
 * host branch's comment (src/main/core/emsmap.c), the EMSDRV assembly
 * routines of segment 069d; pointer arguments are DS offsets:
 *   0 present()                    069d:000a opeu_ems_present
 *   1 status()                     069d:002a opeu_ems_status
 *   2 page_count(&count)           069d:004b opeu_ems_page_count
 *   3 free_pages(&total, &free)    069d:0064 opeu_ems_free_pages
 *   4 frame(&segment)              069d:0083 opeu_ems_frame
 *   5 allocate(pages, &handle)     069d:009d opeu_ems_allocate
 *   6 page_array(pages)            069d:00b8 opeu_ems_page_array
 *   7 map(handle, physical, logical) 069d:00cf opeu_ems_map
 *   8 release(handle)              069d:00e5 opeu_ems_release
 * SEGSETUP passes its own event to the EMS routines and forwards ids 0..8
 * here (link_emsmap). */
#include "link.h"

uint16_t link_emsmap(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    (void)ds;
    switch (id) {
    case 0: return (uint16_t)opeu_ems_present();
    case 1: return (uint16_t)opeu_ems_status();
    case 2: return (uint16_t)opeu_ems_page_count((int16_t *)PORT_DS(ARG(0)));
    case 3: return (uint16_t)opeu_ems_free_pages((int16_t *)PORT_DS(ARG(0)), (int16_t *)PORT_DS(ARG(1)));
    case 4: return (uint16_t)opeu_ems_frame((uint16_t *)PORT_DS(ARG(0)));
    case 5: return (uint16_t)opeu_ems_allocate((int16_t)ARG(0), (int16_t *)PORT_DS(ARG(1)));
    case 6: return (uint16_t)opeu_ems_page_array(PORT_DS(ARG(0)));
    case 7: return (uint16_t)opeu_ems_map((int16_t)ARG(0), ARG(1), (int16_t)ARG(2));
    case 8: return (uint16_t)opeu_ems_release((int16_t)ARG(0));
    default: port_unhandled("emsmap", id, count);
    }
    return 0;
}

