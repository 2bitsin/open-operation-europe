/* Link: recovered/io/osound.cpp (OPEN 0349:0002..02b1, the INT 66h music
 * interface; MAIN SOUND lineage). The driver open is OSYSLIB 0000:13e0,
 * OPEN-only (services/open-syslib.cpp); the bank is 0710:0002 of OPEN's image
 * (memory.cpp). The int86 sites issue AH = 1 (start), 2 (stop), 3 (status)
 * and 4; each goes to the same resident driver instance MAIN uses afterwards
 * (port_fm_int66, services/fm.cpp), as FMDRV stayed resident between the two
 * programs.
 */
#include "open-services.hpp"
#include "port.hpp"
#include "process.hpp"
#include "services.hpp"

namespace opeu::intro {
namespace {
auto osound_int66(uint16_t* w, uint16_t* ds_seg) -> uint16_t {
  port_fm_int66(&w[0], &w[1], &w[3], ds_seg);
  w[6] = 0;
  return w[0];
}
}

auto link_osound([[maybe_unused]] uint8_t* ds, SoundServiceId call, uint16_t* words) -> uint16_t {
  using enum SoundServiceId;
  switch (call) {
  case DRIVER_OPEN:
    if (words[1] != BANK_FRAME) port_fatal("OPEN: sound driver opened with bank %04x:%04x", words[1], words[0]);
    return static_cast<uint16_t>(opeu_open_sound_open(words[0], OPEN_BANK_SEGMENT));
  case INT86: {
    uint16_t ds_seg = OPEN_DS_SEGMENT;
    return osound_int66(words, &ds_seg);
  }
  case INT86X: return osound_int66(words, &words[10]);
  case SEGREAD:
    words[0] = OPEN_DS_SEGMENT;      /* ES */
    words[1] = OPEN_CODE_0349;       /* CS */
    words[2] = OPEN_DS_SEGMENT;      /* SS */
    words[3] = OPEN_DS_SEGMENT;      /* DS */
    return 0;
  default: port_unhandled("osound", static_cast<unsigned>(call), 0);
  }
  return 0;
}

}
