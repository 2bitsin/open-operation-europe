/* Link: recovered/core/openmain.cpp (OPEN 03ac:0004..2817, the opening's
 * scenes and main()). Each service performs the original callee: a recovered
 * OPEN routine (its entry with that module's own link callback) or the
 * native replacement of an assembly / DOS / driver service (OSYSLIB,
 * OVGALIB, OFILEIO and OCOMPOS are OPEN's copies of MAIN's KOEI library
 * segments; services/open-*.cpp hold the OPEN-only ones). Argument
 * conversion only. The library services and their MAIN originals:
 *   SFX_PLAY, SFX_BUSY     OSYSLIB, OPEN-only (int 66h F001h/F000h, counter A)
 *   FM_BUSY                OSYSLIB = MAIN 405F (counter B)
 *   TIMELINE_START         OSYSLIB = MAIN 4052 (F101h, counter B)
 *   FM_NOOP                OSYSLIB = MAIN 4048
 *   INSTALL_DOS_HANDLERS   OSYSLIB = MAIN 402E
 *   VGA_CLEAR              OVGALIB = MAIN 2AAC
 *   DRAW_FAR, BLIT_FAR     OVGALIB = MAIN 28B0, 2674
 *   UI_BOX                 OVGALIB = MAIN 2AD2
 *   STUB_09B7              OVGALIB (empty)
 *   SCREEN_COPY            OCOMPOS = MAIN 301F text; the other OCOMPOS
 *                          routines are OPEN-only
 *   DOS_*                  OFILEIO = MAIN FILEIO; DOS_ALLOC and DOS_FREE
 *                          work in OPEN's process
 *   INT86                  C library; the opening issues only INT 66h (the
 *                          shutdown's AH=1 AL=5 and AH=2 AL=20)
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

namespace opeu::intro {
namespace {
auto source_bytes(uint16_t segment, uint16_t offset, uint32_t size) -> const uint8_t* {
  static uint8_t wrapped[0x10000];
  uint32_t       i = 0;
  if (static_cast<uint32_t>(offset) + size <= 0x10000u) return port_far(segment, offset);
  for (i = 0; i < size && i < sizeof wrapped; i++) wrapped[i] = *port_far(segment, static_cast<uint16_t>(offset + i));
  return wrapped;
}

/* Rows of the blitters' sources: blit4 (2674) reads 4 bytes per byte column,
 * draw1 (28B0) one; row doubling (mode bit 15) reads each row once. */
auto blit_size(uint16_t w, uint16_t h, int planes) -> uint32_t {
  return static_cast<uint32_t>(w) * h * static_cast<uint32_t>(planes);
}

/* INT 66h from OPENMAIN's int86: AX from the REGS block; BX, DX and DS are
 * not read by the two functions it issues (AH=1 start, AH=2 stop). */
auto openmain_int86(uint16_t number, uint16_t ax) -> uint16_t {
  uint16_t bx     = 0;
  uint16_t dx     = 0;
  uint16_t ds_seg = OPEN_DS_SEGMENT;
  if (number != 0x66) port_fatal("OPEN: int86(%02x) is not issued by the opening", number);
  port_fm_int66(&ax, &bx, &dx, &ds_seg);
  return ax;
}
}

auto link_openmain(uint8_t* ds, OpenEventId id, uint16_t const* args, uint16_t count) -> uint16_t {
  using enum OpenEventId;
  auto const arg{ [&](int i) -> uint16_t { return Arg(args, count, i); } };
  switch (id) {
  case SFX_PLAY:             opeu_open_sfx_play(arg(0)); return 0;
  case SFX_BUSY:             return opeu_open_sfx_busy();
  case FM_BUSY:              return opeu_fm_busy();
  case INPUT_POLL:           return static_cast<uint16_t>(opeu_open_input_poll(ds, link_outil));
  case VGA_CLEAR:            return static_cast<uint16_t>(opeu_vga_clear());
  case PALETTE_FADE:         return static_cast<uint16_t>(opeu_open_palette_fade(ds, link_outil, ds + arg(1), arg(0)));
  case PALETTE_STEP:         return static_cast<uint16_t>(opeu_open_palette_step(ds, link_outil, ds + arg(0)));
  case PALETTE_SET:          opeu_open_palette_set(ds, link_outil, ds + arg(0)); return 0;
  case FADE_OUT:             return static_cast<uint16_t>(opeu_open_fade_in(ds, link_outil));
  case FADE_OUT_WAIT:        return static_cast<uint16_t>(opeu_open_fade_in_wait(ds, link_outil));
  case PICTURE_AT:           opeu_picture_file_at(ds, link_outil, arg(0), arg(1), static_cast<int32_t>(Arg32(args, count, 2))); return 0;
  case GFX_WAIT:             opeu_gfx_wait(ds, link_oemsmap); return 0;
  case MUSIC_START:          opeu_music_start(ds, link_osound, static_cast<uint8_t>(arg(0))); return 0;
  case MUSIC_COMMAND4:       opeu_finish_side_initialization(ds, link_osound, static_cast<uint8_t>(arg(0))); return 0;
  case DOS_SEEK:             return static_cast<uint16_t>(opeu_dos_seek(static_cast<int16_t>(arg(0)), static_cast<int32_t>(Arg32(args, count, 1)), static_cast<int16_t>(arg(3))));
  case DOS_READ_FAR:         return static_cast<uint16_t>(opeu_dos_read_far(static_cast<int16_t>(arg(0)), arg(1), arg(2), arg(3)));
  case DRAW_FAR:             return static_cast<uint16_t>(opeu_draw_far(arg(0), arg(1), arg(2), arg(3), arg(4), source_bytes(arg(5), arg(6), blit_size(arg(2), arg(3), 1))));
  case BLIT_FAR:             return static_cast<uint16_t>(opeu_bitmap_restore(arg(0), arg(1), arg(2), arg(3), arg(4), source_bytes(arg(5), arg(6), blit_size(arg(2), arg(3), 4))));
  case BITMAP_SAVE:          opeu_open_bitmap_save(arg(0), arg(1), arg(2), arg(3), arg(4), arg(5)); return 0;
  case DISSOLVE:             opeu_open_dissolve_blit(arg(0), arg(1), arg(2), arg(3), arg(4)); return 0;
  case SCREEN_COPY:          opeu_screen_copy(arg(0), arg(1), arg(2), arg(3), arg(4), arg(5)); return 0;
  case UI_BOX:               return static_cast<uint16_t>(opeu_ui_box(arg(0), arg(1), arg(2), arg(3), arg(4)));
  case FAR_COPY:             opeu_open_far_copy(arg(0), arg(1), arg(2), arg(3), arg(4)); return 0;
  case BUF_OR:               opeu_open_buf_or(arg(0), arg(1), arg(2), arg(3), arg(4), arg(5)); return 0;
  case BUF_AND_MASK:         opeu_open_buf_and_mask(arg(0), arg(1), arg(2), arg(3), arg(4), arg(5)); return 0;
  case TIMELINE_START:       opeu_fm_wait_start(arg(0)); return 0;
  case FM_NOOP:              opeu_fm_noop(); return 0;
  case INSTALL_DOS_HANDLERS: koei_install_dos_handlers(); return 0;
  case DOS_ALLOC:            return open_dos_alloc(arg(0));
  case DOS_FREE:             return open_dos_free(arg(0)) == 0 ? 1 : 0;
  case DOS_CLOSE:            return static_cast<uint16_t>(opeu_dos_close(static_cast<int16_t>(arg(0))));
  case DOS_EXISTS:           {
    int16_t found = opeu_dos_exists(reinterpret_cast<const char*>(ds) + arg(0));
    /* The shutdown busy-waits for SCESEL.DAT with this call; keep the
         * host responsive while it is missing (no effect on the result). */
    if (found == 0) port_pump();
    return static_cast<uint16_t>(found);
  }
  case STUB_09B7:        koei_09b7(arg(0), arg(1)); return 0;
  case INT86:            return openmain_int86(arg(0), arg(1));
  case UI_TEXT:          opeu_ui_text(ds, link_otext_host(), arg(0)); return 0;
  case CURSOR_PRINT:     link_otext_print(ds, args, 1); return 0;
  case MEMORY_RELEASE:   return static_cast<uint16_t>(opeu_memory_release(ds, link_oemsmap));
  case EMS_ALLOC_SPRITE: {
    uint16_t sp = open_stack_enter(0x10);
    auto     r  = static_cast<uint16_t>(opeu_ems_alloc_sprite(ds, link_oemsmap, sp));
    open_stack_leave(0x10);
    return r;
  }
  case EMS_DETECT: {
    uint16_t sp = open_stack_enter(0x110); /* 00f8: 106h bytes of locals */
    auto     r  = static_cast<uint16_t>(opeu_ems_detect(ds, link_oemsmap, sp));
    open_stack_leave(0x110);
    return r;
  }
  case SOUND_INITIALIZE: opeu_sound_initialize(ds, link_osound); return 0;
  case MUSIC_PLAYING:    return static_cast<uint16_t>(opeu_music_playing(ds, link_osound));
  case MOUSE_RESET:      return opeu_mouse_reset(ds, link_outil);
  case FADE_ALT:         return static_cast<uint16_t>(opeu_open_fade_alt(ds, link_outil));
  case IDENTITY_PALETTE: opeu_open_identity_palette(ds, link_outil); return 0;
  case FILE_OR_DIE:      return static_cast<uint16_t>(opeu_open_file_or_die(ds, link_outil, arg(0)));
  case HANG:             port_pump(); return 0; /* the stop after an allocation failure */
  default:               port_unhandled("openmain", static_cast<unsigned>(id), count);
  }
  return 0;
}

}
