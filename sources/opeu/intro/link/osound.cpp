/* Link: open/io/osound.c (OPEN 0349:0002..02b1, the INT 66h music
 * interface; MAIN SOUND lineage). Service calls, from the host branch:
 *   0 driver open (bank offset, segment relative to the load segment):
 *     0000:13e0 OSYSLIB, OPEN-only (port/services/open_syslib.c); the bank
 *     is 0710:0002 of OPEN's image (port/open/open_memory.c)
 *   1 int86(0x66): seven REGS words in/out, returns AX
 *   2 int86x(0x66): REGS, then es cs ss ds in/out
 *   3 segread: es cs ss ds of OPEN (SS = DS = DGROUP)
 * The int86 sites issue AH = 1 (start), 2 (stop), 3 (status) and 4; each
 * goes to the same resident driver instance MAIN uses afterwards
 * (port_fm_int66, port/services/fm.c), as FMDRV stayed resident between the
 * two programs.
 */
#include "open-services.hpp"
#include "port.hpp"
#include "process.hpp"
#include "services.hpp"

static uint16_t osound_int66(uint16_t* w, uint16_t* ds_seg) {
  port_fm_int66(&w[0], &w[1], &w[3], ds_seg);
  w[6] = 0;
  return w[0];
}

uint16_t link_osound(uint8_t* ds, uint16_t call, uint16_t* words) {
  (void)ds;
  switch (call) {
  case 0:
    if (words[1] != 0x0710) port_fatal("OPEN: sound driver opened with bank %04x:%04x", words[1], words[0]);
    return (uint16_t)opeu_open_sound_open(words[0], OPEN_BANK_SEGMENT);
  case 1: {
    uint16_t ds_seg = OPEN_DS_SEGMENT;
    return osound_int66(words, &ds_seg);
  }
  case 2: return osound_int66(words, &words[10]);
  case 3:
    words[0] = OPEN_DS_SEGMENT;      /* ES */
    words[1] = OPEN_CODE_0349;       /* CS */
    words[2] = OPEN_DS_SEGMENT;      /* SS */
    words[3] = OPEN_DS_SEGMENT;      /* DS */
    return 0;
  default: port_unhandled("osound", call, 0);
  }
  return 0;
}
