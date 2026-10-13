/* OPEN.EXE as a process in the port: the runner and its memory.
 *
 * Memory. OPEN is a separate DOS program: its own DGROUP (paragraph 093A of
 * its load image), its far data (the song bank, frame 0710) and its own DOS
 * memory blocks. While it runs, the port's data arena (port_arena, 640 KiB
 * of conventional memory, plain data) holds OPEN's process instead of MAIN's:
 *
 *   OPEN_LOAD_SEGMENT        load image (its code is native; nothing there)
 *   OPEN_BANK_SEGMENT:0002   the FM song bank (recovered/data/ofmusic.cpp)
 *   OPEN_DS_SEGMENT          DGROUP (the five recovered/data modules)
 *   OPEN_HEAP_SEGMENT ..     the DOS blocks OPEN allocates (443 KiB)
 *
 * The load segment is the port's choice (a DOS machine gives a different
 * one, depending on what is resident); OPEN only ever stores the segments
 * DOS returns and computes nothing from them. When OPEN has ended, the
 * arena is rebuilt with MAIN's image (port_memory_init), as DOS loaded
 * MAIN.EXE over the memory OPEN had freed.
 *
 * Stack. MS C's start-up runs OPEN with SS = DS and SP = 3040h (stack
 * segment 0AFE:1400 of the EXE header: 0AFE - 093A = 1C4h paragraphs). The
 * routines that keep stack buffers at DS addresses (the text engine's
 * conversion buffer, the variadic print
 * arguments) reserve them on OpenState's stack.
 */
#pragma once

#include "recovered.hpp"

#include <cstdint>
#include <span>

namespace opeu::intro {

inline constexpr uint16_t OPEN_LOAD_SEGMENT{ 0x0000 };
inline constexpr uint16_t OPEN_BANK_SEGMENT{ OPEN_LOAD_SEGMENT + BANK_FRAME };
inline constexpr uint16_t OPEN_DS_SEGMENT{ OPEN_LOAD_SEGMENT + 0x093a };
inline constexpr uint16_t OPEN_HEAP_SEGMENT{ OPEN_DS_SEGMENT + (OPEN_STACK_TOP >> 4) + 1 };

/* Clear the arena and place OPEN's initial DGROUP and far data. */
void open_memory_init();
/* INT 21h AH=48h/49h in OPEN's process (first fit, one MCB paragraph per block). */
auto open_dos_alloc(uint16_t paragraphs) -> uint16_t;
auto open_dos_free(uint16_t segment) -> int;

/* The placement of the recovered OPEN data objects (image.cpp). */
struct OpenImageObject {
  void const* object;
  uint16_t    seg, off; /* seg: 0 = DGROUP, else relative to the load segment */
  uint16_t    size;
  char const* name;
};
extern OpenImageObject const open_image_objects[];
extern unsigned const        open_image_object_count;

}
