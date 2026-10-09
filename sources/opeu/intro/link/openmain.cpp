/* Link: open/core/openmain.c (OPEN 03ac:0004..2817, the opening's scenes
 * and main()). Services, from the host branch's comments; each performs
 * the original callee: a recovered OPEN routine (its host entry with that
 * module's own link callback) or the native replacement of an assembly /
 * DOS / driver service. Argument conversion only.
 *
 *  0 sfx_play(ticks)        0000:133e OSYSLIB  INT 66h F001h (counter A)
 *  1 sfx_busy() -> AX       0000:134b OSYSLIB  INT 66h F000h
 *  2 fm_busy() -> AX        0000:1337 OSYSLIB = MAIN 405F (counter B)
 *  3 input_poll() -> AX     062d:032e OUTIL
 *  4 vga_clear()            0000:0913 OVGALIB = MAIN 2AAC
 *  5 palette_fade(mode, p)  062d:0262 OUTIL (target, mode)
 *  6 palette_step(p)        062d:01fe OUTIL
 *  7 palette_set(p)         062d:01bc OUTIL
 *  8 fade_out()             062d:0304 OUTIL
 *  9 fade_out_wait()        062d:0310 OUTIL
 * 10 picture_at(x, y, lo, hi) 062d:018e OUTIL
 * 11 gfx_wait()             032a:0048 OEMSMAP
 * 12 music_start(track)     0349:00a4 OSOUND
 * 13 music_command4(v)      0349:0020 OSOUND
 * 14 dos_seek(h, lo, hi, o) 0000:009b OFILEIO = MAIN FILEIO
 * 15 dos_read_far(h, s, o, n) 0000:0057 OFILEIO = MAIN FILEIO
 * 16 draw_far(x,y,w,h,mode,s,o)  0000:0688 OVGALIB = MAIN 28B0 (opeu_draw_far)
 * 17 blit_far(x,y,w,h,mode,s,o)  0000:044c OVGALIB = MAIN 2674 (opeu_bitmap_restore)
 * 18 bitmap_save(x,y,w,h,s,o)    0000:1b56 OCOMPOS (port/services/open_compos.c)
 * 19 dissolve(x,y,s,o,pattern)   0000:1e2f OCOMPOS
 * 20 screen_copy(x,y,w,h,tx,ty)  0000:1bb9 OCOMPOS = MAIN 301F text
 * 21 ui_box(x,y,w,h,c)      0000:0939 OVGALIB = MAIN 2AD2
 * 22 far_copy(ss,so,s,o,n)  0000:1c2f OCOMPOS
 * 23 buf_or(s,o,w,h,ss,so)  0000:1c74 OCOMPOS
 * 24 buf_and_mask(s,o,w,h,ms,mo) 0000:1c94 OCOMPOS
 * 25 timeline_start(ticks)  0000:132a OSYSLIB = MAIN 4052 (F101h, counter B)
 * 26 fm_noop()              0000:1352 OSYSLIB = MAIN 4048
 * 27 install_dos_handlers() 0000:1310 OSYSLIB = MAIN 402E
 * 28 dos_alloc(paragraphs)  0000:0107 OFILEIO = MAIN FILEIO, OPEN's process
 * 29 dos_free(segment)      0000:011a OFILEIO, OPEN's process
 * 30 dos_close(handle)      0000:0032 OFILEIO = MAIN FILEIO
 * 31 dos_exists(name)       0000:00d9 OFILEIO = MAIN FILEIO
 * 32 stub_09b7(1, 0)        0000:09b7 OVGALIB (empty)
 * 33 int86(n, AX)           0000:1f40 C library: only INT 66h (the shutdown's
 *                           AH=1 AL=5 and AH=2 AL=20)
 * 34 ui_text(text)          02a0:071e OTEXT
 * 35 cursor_print(text)     02a0:073e OTEXT (one argument word)
 * 36 memory_release()       032a:005c OEMSMAP
 * 37 ems_alloc_sprite()     032a:00d0 OEMSMAP
 * 38 ems_detect()           032a:00f8 OEMSMAP
 * 39 sound_initialize()     0349:0002 OSOUND
 * 40 music_playing()        0349:006e OSOUND
 * 41 mouse_reset()          062d:0094 OUTIL
 * 42 fade_alt()             062d:031c OUTIL
 * 43 identity_palette()     062d:0328 OUTIL
 * 44 file_or_die(name)      062d:0350 OUTIL
 *
 * Far sources of the two blitters: the original reads them with LODS
 * through DS:SI, which wraps inside the segment; source_bytes() hands the
 * native blitter the same bytes (a wrapped copy when a rectangle would run
 * past offset FFFFh; OPEN's own rectangles never do).
 */
#include "open-services.hpp"
#include "port.hpp"
#include "process.hpp"
#include "services.hpp"

#define ARG(i) ((uint16_t)((i) < count ? args[(i)] : 0))
#define ARG32(i) ((uint32_t)ARG(i) | (uint32_t)ARG((i) + 1) << 16)

static const uint8_t* source_bytes(uint16_t segment, uint16_t offset, uint32_t size) {
  static uint8_t wrapped[0x10000];
  uint32_t i;
  if ((uint32_t)offset + size <= 0x10000u) return port_far(segment, offset);
  for (i = 0; i < size && i < sizeof wrapped; i++) wrapped[i] = *port_far(segment, (uint16_t)(offset + i));
  return wrapped;
}

/* Rows of the blitters' sources: blit4 (2674) reads 4 bytes per byte column,
 * draw1 (28B0) one; row doubling (mode bit 15) reads each row once. */
static uint32_t blit_size(uint16_t w, uint16_t h, int planes) {
  return (uint32_t)w * h * (uint32_t)planes;
}

/* INT 66h from OPENMAIN's int86: AX from the REGS block; BX, DX and DS are
 * not read by the two functions it issues (AH=1 start, AH=2 stop). */
static uint16_t openmain_int86(uint16_t number, uint16_t ax) {
  uint16_t bx = 0, dx = 0, ds_seg = OPEN_DS_SEGMENT;
  if (number != 0x66) port_fatal("OPEN: int86(%02x) is not issued by the opening", number);
  port_fm_int66(&ax, &bx, &dx, &ds_seg);
  return ax;
}

uint16_t link_openmain(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  switch (id) {
  case 0: opeu_open_sfx_play(ARG(0)); return 0;
  case 1: return opeu_open_sfx_busy();
  case 2: return opeu_fm_busy();
  case 3: return (uint16_t)opeu_open_input_poll(ds, link_outil);
  case 4: return (uint16_t)opeu_vga_clear();
  case 5: return (uint16_t)opeu_open_palette_fade(ds, link_outil, ds + ARG(1), ARG(0));
  case 6: return (uint16_t)opeu_open_palette_step(ds, link_outil, ds + ARG(0));
  case 7: opeu_open_palette_set(ds, link_outil, ds + ARG(0)); return 0;
  case 8: return (uint16_t)opeu_open_fade_in(ds, link_outil);
  case 9: return (uint16_t)opeu_open_fade_in_wait(ds, link_outil);
  case 10: opeu_picture_file_at(ds, link_outil, ARG(0), ARG(1), (int32_t)ARG32(2)); return 0;
  case 11: opeu_gfx_wait(ds, link_oemsmap); return 0;
  case 12: opeu_music_start(ds, link_osound, (uint8_t)ARG(0)); return 0;
  case 13: opeu_finish_side_initialization(ds, link_osound, (uint8_t)ARG(0)); return 0;
  case 14: return (uint16_t)opeu_dos_seek((int16_t)ARG(0), (int32_t)ARG32(1), (int16_t)ARG(3));
  case 15: return (uint16_t)opeu_dos_read_far((int16_t)ARG(0), ARG(1), ARG(2), ARG(3));
  case 16:
    return (uint16_t)opeu_draw_far(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4),
                                   source_bytes(ARG(5), ARG(6), blit_size(ARG(2), ARG(3), 1)));
  case 17:
    return (uint16_t)opeu_bitmap_restore(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4),
                                         source_bytes(ARG(5), ARG(6), blit_size(ARG(2), ARG(3), 4)));
  case 18: opeu_open_bitmap_save(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5)); return 0;
  case 19: opeu_open_dissolve_blit(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4)); return 0;
  case 20: opeu_screen_copy(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5)); return 0;
  case 21: return (uint16_t)opeu_ui_box(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4));
  case 22: opeu_open_far_copy(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4)); return 0;
  case 23: opeu_open_buf_or(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5)); return 0;
  case 24: opeu_open_buf_and_mask(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5)); return 0;
  case 25: opeu_fm_wait_start(ARG(0)); return 0;
  case 26: opeu_fm_noop(); return 0;
  case 27: koei_install_dos_handlers(); return 0;
  case 28: return open_dos_alloc(ARG(0));
  case 29: return open_dos_free(ARG(0)) == 0 ? 1 : 0;
  case 30: return (uint16_t)opeu_dos_close((int16_t)ARG(0));
  case 31: {
    int16_t found = opeu_dos_exists((const char*)ds + ARG(0));
    /* The shutdown busy-waits for SCESEL.DAT with this call; keep the
         * host responsive while it is missing (no effect on the result). */
    if (!found) port_pump();
    return (uint16_t)found;
  }
  case 32: koei_09b7(ARG(0), ARG(1)); return 0;
  case 33: return openmain_int86(ARG(0), ARG(1));
  case 34: opeu_ui_text(ds, link_otext_host(), ARG(0)); return 0;
  case 35: link_otext_print(ds, args, 1); return 0;
  case 36: return (uint16_t)opeu_memory_release(ds, link_oemsmap);
  case 37: {
    uint16_t sp = open_stack_enter(0x10);
    uint16_t r = (uint16_t)opeu_ems_alloc_sprite(ds, link_oemsmap, sp);
    open_stack_leave(0x10);
    return r;
  }
  case 38: {
    uint16_t sp = open_stack_enter(0x110);    /* 00f8: 106h bytes of locals */
    uint16_t r = (uint16_t)opeu_ems_detect(ds, link_oemsmap, sp);
    open_stack_leave(0x110);
    return r;
  }
  case 39: opeu_sound_initialize(ds, link_osound); return 0;
  case 40: return (uint16_t)opeu_music_playing(ds, link_osound);
  case 41: return opeu_mouse_reset(ds, link_outil);
  case 42: return (uint16_t)opeu_open_fade_alt(ds, link_outil);
  case 43: opeu_open_identity_palette(ds, link_outil); return 0;
  case 44: return (uint16_t)opeu_open_file_or_die(ds, link_outil, ARG(0));
  case 45: port_pump(); return 0;               /* HANG(): the stop after an allocation failure */
  default: port_unhandled("openmain", id, count);
  }
  return 0;
}
