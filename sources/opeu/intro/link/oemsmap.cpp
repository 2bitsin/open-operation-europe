/* Link: recovered/core/oemsmap.cpp (OPEN 032a:000e..01f1, EMS for the
 * sprite buffer). Events: the EMSDRV routines of OPEN segment 031c, MAIN's
 * EMSDRV linked unchanged into OPEN, so the native EMSDRV of MAIN
 * (services/ems.cpp: no expanded memory manager) serves them. Pointer
 * arguments are DS offsets of OPEN's DGROUP. With no EMM, detection fails at
 * present() and the opening allocates its sixth buffer in DOS memory, as on
 * a PC without EMS.
 */
#include "port.hpp"
#include "process.hpp"
#include "services.hpp"

namespace opeu::intro {

uint16_t link_oemsmap(uint8_t* ds, EmsEventId id, uint16_t const* args, uint16_t count) {
  using enum EmsEventId;
  auto const arg{ [&](int i) { return Arg(args, count, i); } };
  switch (id) {
  case PRESENT: return (uint16_t)opeu_ems_present();
  case STATUS: return (uint16_t)opeu_ems_status();
  case PAGE_COUNT: return (uint16_t)opeu_ems_page_count((int16_t*)(ds + arg(0)));
  case FREE_PAGES: return (uint16_t)opeu_ems_free_pages((int16_t*)(ds + arg(0)), (int16_t*)(ds + arg(1)));
  case FRAME: return (uint16_t)opeu_ems_frame((uint16_t*)(ds + arg(0)));
  case ALLOCATE: return (uint16_t)opeu_ems_allocate((int16_t)arg(0), (int16_t*)(ds + arg(1)));
  case PAGE_ARRAY: return (uint16_t)opeu_ems_page_array(ds + arg(0));
  case MAP: return (uint16_t)opeu_ems_map((int16_t)arg(0), arg(1), (int16_t)arg(2));
  case RELEASE: return (uint16_t)opeu_ems_release((int16_t)arg(0));
  default: port_unhandled("oemsmap", static_cast<unsigned>(id), count);
  }
  return 0;
}

}
