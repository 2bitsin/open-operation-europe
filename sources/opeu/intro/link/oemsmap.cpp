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

auto link_oemsmap(uint8_t* ds, EmsEventId id, uint16_t const* args, uint16_t count) -> uint16_t {
  using enum EmsEventId;
  auto const arg{ [&](int i) -> uint16_t { return Arg(args, count, i); } };
  switch (id) {
  case PRESENT:    return static_cast<uint16_t>(opeu_ems_present());
  case STATUS:     return static_cast<uint16_t>(opeu_ems_status());
  case PAGE_COUNT: return static_cast<uint16_t>(opeu_ems_page_count(reinterpret_cast<int16_t*>(ds + arg(0))));
  case FREE_PAGES: return static_cast<uint16_t>(opeu_ems_free_pages(reinterpret_cast<int16_t*>(ds + arg(0)), reinterpret_cast<int16_t*>(ds + arg(1))));
  case FRAME:      return static_cast<uint16_t>(opeu_ems_frame(reinterpret_cast<uint16_t*>(ds + arg(0))));
  case ALLOCATE:   return static_cast<uint16_t>(opeu_ems_allocate(static_cast<int16_t>(arg(0)), reinterpret_cast<int16_t*>(ds + arg(1))));
  case PAGE_ARRAY: return static_cast<uint16_t>(opeu_ems_page_array(ds + arg(0)));
  case MAP:        return static_cast<uint16_t>(opeu_ems_map(static_cast<int16_t>(arg(0)), arg(1), static_cast<int16_t>(arg(2))));
  case RELEASE:    return static_cast<uint16_t>(opeu_ems_release(static_cast<int16_t>(arg(0))));
  default:         port_unhandled("oemsmap", static_cast<unsigned>(id), count);
  }
  return 0;
}

}
