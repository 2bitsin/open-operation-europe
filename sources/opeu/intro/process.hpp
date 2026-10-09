/* OPEN.EXE as a process in the port (the runner, its memory and its link
 * layer).
 *
 * Memory. OPEN is a separate DOS program: its own DGROUP (paragraph 093A of
 * its load image), its far data (the song bank, frame 0710) and its own DOS
 * memory blocks. While it runs, the port's data arena (port_arena, 640 KiB
 * of conventional memory, plain data) holds OPEN's process instead of MAIN's:
 *
 *   OPEN_LOAD_SEGMENT        load image (its code is native; nothing there)
 *   OPEN_BANK_SEGMENT:0002   the FM song bank (recovered/data/ofmusic.cpp)
 *   OPEN_DS_SEGMENT          DGROUP (the six recovered/data modules)
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
 * conversion buffer, the EMS detection's locals, the variadic print
 * arguments) get them below open_sp, like MAIN's port_sp.
 */
#pragma once

#include "recovered.hpp"

#include <cstdint>

namespace opeu::intro {

inline constexpr uint16_t OPEN_LOAD_SEGMENT{ 0x0000 };
inline constexpr uint16_t OPEN_BANK_SEGMENT{ OPEN_LOAD_SEGMENT + BANK_FRAME };
inline constexpr uint16_t OPEN_DS_SEGMENT{ OPEN_LOAD_SEGMENT + 0x093a };
inline constexpr uint16_t OPEN_CODE_0349{ OPEN_LOAD_SEGMENT + 0x0349 };    /* OSOUND's CS (segread) */
inline constexpr uint16_t OPEN_DS_END{ sizeof(OpenDataSegment) };          /* _end: past DGROUP data + BSS */
inline constexpr uint16_t OPEN_STACK_TOP{ 0x3040 };                        /* SS = DS, SP after crt0 */
inline constexpr uint16_t OPEN_HEAP_SEGMENT{ OPEN_DS_SEGMENT + (OPEN_STACK_TOP >> 4) + 1 };

extern uint8_t* open_ds;    /* port_arena + OPEN_DS_SEGMENT * 16 */
extern uint16_t open_sp;

/* Clear the arena and place OPEN's initial DGROUP and far data. */
void open_memory_init(void);
/* INT 21h AH=48h/49h in OPEN's process (first fit, one MCB paragraph per block). */
uint16_t open_dos_alloc(uint16_t paragraphs);
int open_dos_free(uint16_t segment);
/* Modeled stack (see port_stack_enter in port.hpp). */
uint16_t open_stack_enter(uint16_t bytes);
void open_stack_leave(uint16_t bytes);

/* The placement of the recovered OPEN data objects (image.cpp). */
struct OpenImageObject {
  void const* object;
  uint16_t seg, off;    /* seg: 0 = DGROUP, else relative to the load segment */
  uint16_t size;
  char const* name;
};
extern OpenImageObject const open_image_objects[];
extern unsigned const open_image_object_count;

/* Argument word i of a service call with count words (0 past the end). */
inline auto Arg(uint16_t const* args, uint16_t count, int i) -> uint16_t {
  return static_cast<uint16_t>(i < count ? args[i] : 0);
}
/* The 32-bit argument in words i (low) and i + 1 (high). */
inline auto Arg32(uint16_t const* args, uint16_t count, int i) -> uint32_t {
  return static_cast<uint32_t>(Arg(args, count, i)) | static_cast<uint32_t>(Arg(args, count, i + 1)) << 16;
}

/* Service callbacks of the recovered OPEN modules (link/). */
uint16_t link_openmain(uint8_t* ds, OpenEventId id, uint16_t const* args, uint16_t count);
uint32_t link_outil(uint8_t* ds, UtilServiceId id, uint16_t* words, uint16_t count);
uint16_t link_otext(uint8_t* ds, TextEventId id, uint16_t const* args, uint16_t count);
uint16_t link_oemsmap(uint8_t* ds, EmsEventId id, uint16_t const* args, uint16_t count);
uint16_t link_osound(uint8_t* ds, SoundServiceId call, uint16_t* words);
/* OTEXT's host structure (its event and the modeled stack pointer). */
TextHost* link_otext_host(void);
/* cursor_print with the given argument words pushed on the modeled stack. */
void link_otext_print(uint8_t* ds, uint16_t const* words, unsigned count);

}
