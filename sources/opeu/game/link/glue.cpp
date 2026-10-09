/* Platform glue of the game's services (declarations and rationale:
 * port/include/glue.h). Moved here unchanged from the former port/link
 * helpers (a_util.h, b1_util.h, core/b2_common.h, core/c1_common.h,
 * core/c2_common.h, core/c2_air.h, core/c2_msn.h, core/scensel.c,
 * core/notice.c, map/btl_link.h, map/map_common.h, units/units_F.h,
 * units/xferpnl.c, io/mouse.c, io/sound.c, routines/scenbrf.c, aiops.c);
 * token-identical copies are merged (port_scratch_*, port_frame_*,
 * port_rand_bounded, port_daily_show). Stack bookkeeping, argument word
 * placement and register blocks only: no game logic. */
#include "glue.hpp"

#include <stdlib.h>
#include <string.h>

#include "services.hpp"
#include "core/aiops1.hpp"
#include "core/dialogs.hpp"
#include "core/random.hpp"
#include "core/scenbrf.hpp"
#include "core/textout.hpp"
#include "io/keyboard.hpp"
#include "io/loadscen.hpp"
#include "io/resopen.hpp"
#include "map/cellpred.hpp"

/* ---- modeled DGROUP stack ------------------------------------------------ */

#define PORT_C2_AIR_FRAME \
  0x200u /* air modules: objective_damage's BP-0x9e buffers + callee frames */
#define PORT_BTL_RESERVE \
  0x200u /* battle: deepest documented buffers and nested frames < 0x180 */
#define PORT_MAP_FRAME \
  0x300u /* map: scroll_to -> scroll -> cell_draw -> unit_draw ~0x150 */
#define PORT_LF_FRAME    0x200u /* units: locals plus internal sp-n frames */
#define PORT_KEY_SCRATCH 16u    /* keyboard.c: 16-byte caller scratch area */
#define PORT_CLOSING_BUFFER \
  100u /* scenbrf.c: the briefing's closing-text buffer */

uint16_t a_push_words(const uint16_t* args, unsigned count) {
  uint16_t at = (uint16_t)(port_stack_enter((uint16_t)(2 * count)) - 2 * count);
  unsigned i;
  for (i = 0; i < count; i++)
    port_ds_set_word((uint16_t)(at + 2 * i), args[i]);
  return at;
}

void a_pop_words(unsigned count) {
  port_stack_leave((uint16_t)(2 * count));
}

uint16_t b1_stack_alloc(uint16_t n) {
  return (uint16_t)(port_stack_enter(n) - n);
}

uint16_t port_scratch_enter(uint16_t bytes) {
  port_stack_enter(bytes);
  return (uint16_t)(port_sp + 4u);
}

void port_scratch_leave(uint16_t n) {
  port_stack_leave(n);
}

uint16_t b1_frame_enter(void) {
  return (uint16_t)(port_stack_enter(PORT_FRAME) - 2);
}

uint16_t port_frame_enter(void) {
  return (uint16_t)(port_stack_enter(PORT_FRAME) - 2u);
}

void port_frame_leave(void) {
  port_stack_leave(PORT_FRAME);
}

uint16_t b1_sp_enter(void) {
  return port_stack_enter(PORT_FRAME);
}

uint16_t c2_air_sp_enter(void) {
  return port_stack_enter(PORT_C2_AIR_FRAME);
}

uint16_t c2_air_bp_enter(void) {
  return (uint16_t)(port_stack_enter(PORT_C2_AIR_FRAME) - 2u);
}

void c2_air_leave(void) {
  port_stack_leave(PORT_C2_AIR_FRAME);
}

uint16_t btl_sp_enter(void) {
  return port_stack_enter(PORT_BTL_RESERVE);
}

void btl_sp_leave(void) {
  port_stack_leave(PORT_BTL_RESERVE);
}

uint16_t map_frame_enter(void) {
  return (uint16_t)(port_stack_enter(PORT_MAP_FRAME) - 2u);
}

void map_frame_leave(void) {
  port_stack_leave(PORT_MAP_FRAME);
}

uint16_t b1_push_words(const uint16_t* args, unsigned count) {
  uint16_t at = b1_stack_alloc((uint16_t)(2 * count));
  unsigned i;
  for (i = 0; i < count; i++)
    port_ds_set_word((uint16_t)(at + 2 * i), args[i]);
  return at;
}

/* ---- variadic text routines ------------------------------------------------ *
 * 0480:0b9a request_format (sprintf) and 0480:0bb8 cursor_print (army_print,
 * context_print, turn_label) read the pushed argument words at the DS
 * offset `frame` (textout.c REQUEST_PARAMS / PRINT_PARAMS), first word at
 * the lowest address. The conventions differ in the reserve and in whether
 * the words sit at the reserved base or at SP+4. */

typedef void (*TextFrameRoutine)(uint8_t* ds, TextHost* host, uint16_t frame);

void a_cursor_print(uint8_t* ds, const uint16_t* words, unsigned count) {
  uint16_t frame = a_push_words(words, count);
  opeu_cursor_print(ds, (TextHost*)port_textout_host(), frame);
  a_pop_words(count);
}

void b1_cursor_print(uint8_t* ds, const uint16_t* args, unsigned count) {
  uint16_t frame = b1_push_words(args, count);
  opeu_cursor_print(ds, (TextHost*)port_textout_host(), frame);
  port_scratch_leave((uint16_t)(2 * count));
}

void b1_request_format(uint8_t* ds, const uint16_t* args, unsigned count) {
  uint16_t frame = b1_push_words(args, count);
  opeu_request_format(ds, (TextHost*)port_textout_host(), frame);
  port_scratch_leave((uint16_t)(2 * count));
}

/* words at the reserved base */
static void b2_frame_call(uint8_t* ds, TextFrameRoutine routine,
                          const uint16_t* words, unsigned n) {
  uint16_t bytes = (uint16_t)(2u * n), frame;
  unsigned i;
  frame = (uint16_t)(port_stack_enter(bytes) - bytes);
  for (i = 0; i < n; ++i) {
    ds[(uint16_t)(frame + 2u * i)]      = (uint8_t)words[i];
    ds[(uint16_t)(frame + 2u * i + 1u)] = (uint8_t)(words[i] >> 8);
  }
  routine(ds, (TextHost*)port_textout_host(), frame);
  port_stack_leave(bytes);
}

void b2_request_format(uint8_t* ds, const uint16_t* words, unsigned n) {
  b2_frame_call(ds, opeu_request_format, words, n);
}

void b2_cursor_print(uint8_t* ds, const uint16_t* words, unsigned n) {
  b2_frame_call(ds, opeu_cursor_print, words, n);
}

/* words at SP+4 of `bytes` reserved (c1_, c2_ and map_ conventions) */
static void sp4_frame_call(uint8_t* ds, TextFrameRoutine routine,
                           const uint16_t* words, unsigned n, uint16_t bytes) {
  uint16_t frame;
  unsigned i;
  port_stack_enter(bytes);
  frame = (uint16_t)(port_sp + 4u);
  for (i = 0; i < n; ++i) {
    ds[(uint16_t)(frame + 2u * i)]      = (uint8_t)words[i];
    ds[(uint16_t)(frame + 2u * i + 1u)] = (uint8_t)(words[i] >> 8);
  }
  routine(ds, (TextHost*)port_textout_host(), frame);
  port_stack_leave(bytes);
}

void c1_request_format(uint8_t* ds, const uint16_t* words, unsigned n) {
  sp4_frame_call(ds, opeu_request_format, words, n, (uint16_t)(2u * n));
}

void c1_cursor_print(uint8_t* ds, const uint16_t* words, unsigned n) {
  sp4_frame_call(ds, opeu_cursor_print, words, n, (uint16_t)(2u * n));
}

void c2_request_format(uint8_t* ds, const uint16_t* words, unsigned n) {
  sp4_frame_call(ds, opeu_request_format, words, n, (uint16_t)(2u * n));
}

void c2_cursor_print(uint8_t* ds, const uint16_t* words, unsigned n) {
  sp4_frame_call(ds, opeu_cursor_print, words, n, (uint16_t)(2u * n));
}

void map_request_format(uint8_t* ds, const uint16_t* words, unsigned n) {
  sp4_frame_call(ds, opeu_request_format, words, n, (uint16_t)(2u * n));
}

void map_cursor_print(uint8_t* ds, const uint16_t* words, unsigned n) {
  sp4_frame_call(ds, opeu_cursor_print, words, n, (uint16_t)(2u * n));
}

/* battle: one extra word reserved */
void btl_frame_call(uint8_t* ds, BtlFrameRoutine routine, const uint16_t* words,
                    unsigned n) {
  uint16_t bytes = (uint16_t)(2u * n + 2u), frame;
  unsigned i;
  port_stack_enter(bytes);
  frame = (uint16_t)(port_sp + 4u);
  for (i = 0; i < n; ++i) {
    ds[(uint16_t)(frame + 2u * i)]      = (uint8_t)words[i];
    ds[(uint16_t)(frame + 2u * i + 1u)] = (uint8_t)(words[i] >> 8);
  }
  routine(ds, port_textout_host(), frame);
  port_stack_leave(bytes);
}

static void btl_text_request_format(uint8_t* ds, void* host, uint16_t frame) {
  opeu_request_format(ds, (TextHost*)host, frame);
}

static void btl_text_cursor_print(uint8_t* ds, void* host, uint16_t frame) {
  opeu_cursor_print(ds, (TextHost*)host, frame);
}

void btl_request_format(uint8_t* ds, const uint16_t* words, unsigned n) {
  btl_frame_call(ds, btl_text_request_format, words, n);
}

void btl_cursor_print(uint8_t* ds, const uint16_t* words, unsigned n) {
  btl_frame_call(ds, btl_text_cursor_print, words, n);
}

/* units: the words just below the entry SP of LF_FRAME + 2n reserved */
uint16_t lf_request_format(uint8_t* ds, const uint16_t* args, uint16_t count) {
  uint16_t sp = port_stack_enter((uint16_t)(PORT_LF_FRAME + 2u * count));
  uint16_t at = (uint16_t)(sp - 2u * count);
  uint16_t i;
  for (i = 0; i < count; i++)
    port_ds_set_word((uint16_t)(at + 2u * i), args[i]);
  opeu_request_format(ds, (TextHost*)port_textout_host(), at);
  port_stack_leave((uint16_t)(PORT_LF_FRAME + 2u * count));
  return 0;
}

uint16_t lf_cursor_print(uint8_t* ds, const uint16_t* args, uint16_t count) {
  uint16_t sp = port_stack_enter((uint16_t)(PORT_LF_FRAME + 2u * count));
  uint16_t at = (uint16_t)(sp - 2u * count);
  uint16_t i;
  for (i = 0; i < count; i++)
    port_ds_set_word((uint16_t)(at + 2u * i), args[i]);
  opeu_cursor_print(ds, (TextHost*)port_textout_host(), at);
  port_stack_leave((uint16_t)(PORT_LF_FRAME + 2u * count));
  return 0;
}

/* ---- C runtime random state ------------------------------------------------ *
 * 09f6:00d4 rand_bounded: the recovered generator keeps its state in a
 * global; the original state lives at DS:c6f4 (core/random.c). */
uint16_t port_rand_bounded(uint8_t* ds, uint16_t bound) {
  uint16_t r;
  memcpy(&opeu_random_state, ds + 0xc6f4, 4);
  r = opeu_rand_bounded(bound);
  memcpy(ds + 0xc6f4, &opeu_random_state, 4);
  return r;
}

/* ---- DS words ---------------------------------------------------------------- */

void port_set_word(uint8_t* ds, uint16_t at, uint16_t value) {
  ds[at]                 = (uint8_t)value;
  ds[(uint16_t)(at + 1)] = (uint8_t)(value >> 8);
}

/* Host pointer of a DS word the callee writes (output arguments). */
uint16_t* port_ds_word_ptr(uint16_t off) {
  return (uint16_t*)(void*)PORT_DS(off);
}

/* ---- far code pointers ------------------------------------------------------- */

PortCellPredicate c2_cell_predicate(uint16_t off, uint16_t seg) {
  uint32_t address = (uint32_t)seg * 16u + off;
  if (address == 0x145cu * 16u + 0x1114u)
    return opeu_request_cell_allowed;
  if (address == 0x145cu * 16u + 0x1152u)
    return opeu_arrival_cell_allowed;
  port_fatal("request_cells: predicate %04x:%04x is not a cell predicate", seg,
             off);
  return 0;
}

/* Daily presentation: 1707:03e4 daily_show(id, side, far presenter). The
 * presenter is a (offset, segment) pair; DIALOGS calls it back through its
 * present service, i.e. by routine identifier. */
void port_daily_show(uint8_t* ds, uint16_t id, uint16_t side, uint16_t off,
                     uint16_t seg) {
  opeu_daily_show(ds, (const DialogHost*)port_dialogs_host(), id, (uint8_t)side,
                  (uint32_t)seg << 16 | off);
}

/* The briefings of core/scenbrf.c keep their 100-byte closing-text buffer in
 * the original frame; the host takes its DS offset, reserved here. */
uint32_t port_run_briefing(uint8_t* ds, PortBriefing briefing) {
  uint16_t closing = b1_stack_alloc(PORT_CLOSING_BUFFER);
  briefing(ds, svc_scenbrf, closing);
  port_scratch_leave(PORT_CLOSING_BUFFER);
  return 0;
}

uint16_t btl_reorder(uint8_t* ds, uint32_t address, const uint16_t* args,
                     unsigned count, const unsigned char* order, unsigned n) {
  uint16_t out[8];
  unsigned i;
  for (i = 0; i < n && i < 8; ++i)
    out[i] = order[i] < count ? args[order[i]] : 0;
  return (uint16_t)port_call(ds, address, out, n);
}

uint32_t port_call_named(uint8_t* ds, const char* name, const uint16_t* args,
                         unsigned count) {
  uint32_t address = port_routine_address(name);
  if (!address)
    port_fatal("no routine named %s", name);
  return port_call(ds, address, args, count);
}

/* ---- driver register blocks ------------------------------------------------- */

/* INT 33h: int86 copies the output registers back and returns AX. */
uint16_t port_mouse_int33(uint16_t* w) {
  switch (w[0]) {
    case 0:
      w[0] = port_mouse_reset();
      w[1] = 2; /* BX: number of buttons */
      break;
    case 1: port_mouse_show(); break;
    case 2: port_mouse_hide(); break;
    case 3: port_mouse_state(&w[1], &w[2], &w[3]); break;
    case 4: port_mouse_set_position(w[2], w[3]); break;
    case 7: port_mouse_x_range(w[2], w[3]); break;
    case 8: port_mouse_y_range(w[2], w[3]); break;
    default:
      port_fatal("mouse: INT 33h function %04x is not issued by MAIN", w[0]);
  }
  w[6] = 0; /* cflag: int86 copies CF, clear */
  return w[0];
}

/* INT 66h: AX, BX, DX and DS are the driver's inputs and results; CX, SI,
 * DI come back unchanged; the carry flag is clear. */
uint16_t port_sound_int66(uint16_t* w, uint16_t* ds_seg) {
  port_fm_int66(&w[0], &w[1], &w[3], ds_seg);
  w[6] = 0;
  return w[0];
}

/* ---- keyboard prompts ----------------------------------------------------------- */

uint16_t port_prompt_cancel(uint8_t* ds, uint16_t text, uint16_t max,
                            uint16_t items) {
  uint16_t s = b1_stack_alloc(PORT_KEY_SCRATCH), r;
  r          = opeu_prompt_value_cancel(ds, svc_keyboard, text, max, items, s);
  port_scratch_leave(PORT_KEY_SCRATCH);
  return r;
}

uint16_t port_prompt_required(uint8_t* ds, uint16_t text, uint16_t max,
                              uint16_t items) {
  uint16_t s = b1_stack_alloc(PORT_KEY_SCRATCH), r;
  r = opeu_prompt_value_required(ds, svc_keyboard, text, max, items, s);
  port_scratch_leave(PORT_KEY_SCRATCH);
  return r;
}

uint16_t port_prompt_yes_no(uint8_t* ds, uint16_t text) {
  uint16_t s = b1_stack_alloc(PORT_KEY_SCRATCH), r;
  r          = (uint16_t)opeu_prompt_yes_no(ds, svc_keyboard, text, s);
  port_scratch_leave(PORT_KEY_SCRATCH);
  return r;
}

/* ---- LOADSCEN ------------------------------------------------------------------- *
 * LOADSCEN's host entry (io/loadscen.c) takes the contents of the two files
 * the original opens by name (DS:2e8c opeu_scenario_filename, DS:2ef2
 * opeu_map_filename). Open them as the original does (0977:000e
 * opeu_open_resource) and hand over their whole contents. */
#define SCENARIO_FILENAME 0x2e8cu
#define MAP_FILENAME      0x2ef2u

static uint8_t* read_resource(uint8_t* ds, uint16_t name, size_t* size) {
  int16_t  h   = opeu_open_resource(ds, svc_resopen, name);
  int32_t  end = h < 0 ? 0 : opeu_dos_seek(h, 0, 2);
  uint8_t* data;
  size_t   done = 0;
  if (end < 0)
    end = 0;
  data = (uint8_t*)malloc((size_t)end + 1);
  if (!data)
    port_fatal("scensel: out of memory");
  if (h >= 0) {
    opeu_dos_seek(h, 0, 0);
    while (done < (size_t)end) {
      size_t   chunk = (size_t)end - done;
      uint16_t n;
      if (chunk > 0x8000u)
        chunk = 0x8000u;
      n = (uint16_t)opeu_dos_read(h, data + done,
                                  (uint16_t)chunk); /* AX: unsigned count */
      if (n == 0 || n == 0xffffu)
        break;
      done += n;
    }
    opeu_dos_close(h);
  }
  *size = done;
  return data;
}

void port_load_scenario(uint8_t* ds, uint16_t scenario) {
  size_t   scedata_size, mapinf_size;
  uint8_t* scedata = read_resource(ds, SCENARIO_FILENAME, &scedata_size);
  uint8_t* mapinf  = read_resource(ds, MAP_FILENAME, &mapinf_size);
  /* The host entry returns -1 (and leaves DS untouched) when a file is
     * shorter than the records the original reads; the original has no
     * result, so nothing is reported here beyond a trace. */
  if (opeu_load_scenario(ds, (uint8_t)scenario, scedata, scedata_size, mapinf,
                         mapinf_size) != 0)
    port_trace("link",
               "scensel: scenario %u: SCENARIO/MAP files shorter than the "
               "records read",
               scenario);
  free(scedata);
  free(mapinf);
}

/* ---- boundary pointers ------------------------------------------------------- *
 * The host branches of aiops1..aiops5 call six routines through the global
 * opeu_ai_unrecovered (aiops1.c), written when those callees were not yet
 * recovered; svc_ai_unrecovered (game/core/aiops1.c, native build) maps its
 * ids to the recovered routines. Installed by main() before the entry. */
void port_install_hooks(void) {
  opeu_ai_unrecovered = svc_ai_unrecovered;
}
