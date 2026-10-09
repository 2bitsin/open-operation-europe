/* Platform glue between the game's services and the port layer.
 *
 * game/ resolves the numbered services of every recovered module to direct
 * calls (tools/game-services.json, tools/make_game_tree.py stage 11). What a
 * service needs beyond "call the routine with converted arguments" is not
 * game logic but a platform difference of the 16-bit original, and lives
 * here (port/core/glue.c, port/core/hosts.c):
 *
 *  - the modeled DGROUP stack: the host branches keep the original's stack
 *    buffers at DS addresses, so a caller reserves room below the modeled SP
 *    for a callee's `sp`/`frame`/`scratch` parameter (port.h port_sp);
 *  - the cdecl variadic text routines (0480:0b9a request_format, 0480:0bb8
 *    cursor_print): their argument words are laid out on the modeled stack
 *    as the original pushed them;
 *  - the C runtime random state, kept at DS:c6f4 by the original and in
 *    opeu_random_state by the recovered generator;
 *  - the struct hosts of TEXTOUT, UIPRIM and DIALOGS (stack pointer, memory
 *    and segment bases the host branches read) and NUMDLG's bridge;
 *  - far code pointers read from the recovered data (cell predicates,
 *    picture stream readers), mapped to the C functions they name;
 *  - the INT 33h / INT 66h register blocks of the mouse and FM drivers;
 *  - LOADSCEN's file contents (it takes the two files' bytes, the original
 *    read them by name).
 *
 * Several helpers differ only in where they place the argument words or how
 * much stack they reserve (b1_/b2_/c1_/c2_/btl_/map_/lf_ prefixes: the
 * conventions of the port/link files they come from). They are kept apart
 * on purpose: the placement decides which DGROUP bytes are written, and the
 * native build must stay byte-identical to the one it replaces.
 */
#ifndef PORT_GLUE_H
#define PORT_GLUE_H

#include <stddef.h>
#include <stdint.h>

#include "port.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- modeled DGROUP stack ------------------------------------------------ */

/* Reserve n bytes; their DS offset is the base (b1) or SP+4 (scratch). */
uint16_t b1_stack_alloc(uint16_t n);
uint16_t port_scratch_enter(uint16_t bytes);
void port_scratch_leave(uint16_t bytes);
/* `frame` (entry SP - 2) and `sp` (entry SP) with PORT_FRAME reserved. */
uint16_t b1_frame_enter(void);
uint16_t port_frame_enter(void);
void port_frame_leave(void);
uint16_t b1_sp_enter(void);
/* Air modules: C2_AIR_FRAME (0x200) reserved. */
uint16_t c2_air_sp_enter(void);
uint16_t c2_air_bp_enter(void);
void c2_air_leave(void);
/* Battle modules: BTL_RESERVE (0x200) reserved. */
uint16_t btl_sp_enter(void);
void btl_sp_leave(void);
/* Map modules: MAP_FRAME_BYTES (0x300) reserved. */
uint16_t map_frame_enter(void);
void map_frame_leave(void);
/* Push/pop cdecl argument words on the modeled stack. */
uint16_t a_push_words(const uint16_t *args, unsigned count);
void a_pop_words(unsigned count);
uint16_t b1_push_words(const uint16_t *args, unsigned count);

/* ---- variadic text routines: request_format / cursor_print ---------------- */

void a_cursor_print(uint8_t *ds, const uint16_t *words, unsigned count);
void b1_cursor_print(uint8_t *ds, const uint16_t *args, unsigned count);
void b1_request_format(uint8_t *ds, const uint16_t *args, unsigned count);
void b2_request_format(uint8_t *ds, const uint16_t *words, unsigned n);
void b2_cursor_print(uint8_t *ds, const uint16_t *words, unsigned n);
void c1_request_format(uint8_t *ds, const uint16_t *words, unsigned n);
void c1_cursor_print(uint8_t *ds, const uint16_t *words, unsigned n);
void c2_request_format(uint8_t *ds, const uint16_t *words, unsigned n);
void c2_cursor_print(uint8_t *ds, const uint16_t *words, unsigned n);
void map_request_format(uint8_t *ds, const uint16_t *words, unsigned n);
void map_cursor_print(uint8_t *ds, const uint16_t *words, unsigned n);
uint16_t lf_request_format(uint8_t *ds, const uint16_t *args, uint16_t count);
uint16_t lf_cursor_print(uint8_t *ds, const uint16_t *args, uint16_t count);
/* request_format with the battle modules' reserve (2n+2 bytes). */
typedef void (*BtlFrameRoutine)(uint8_t *ds, void *host, uint16_t frame);
void btl_frame_call(uint8_t *ds, BtlFrameRoutine routine, const uint16_t *words, unsigned n);
void btl_request_format(uint8_t *ds, const uint16_t *words, unsigned n);
void btl_cursor_print(uint8_t *ds, const uint16_t *words, unsigned n);

/* ---- C runtime random state (DS:c6f4) ------------------------------------- */

uint16_t port_rand_bounded(uint8_t *ds, uint16_t bound);

/* ---- DS words --------------------------------------------------------------- */

void port_set_word(uint8_t *ds, uint16_t at, uint16_t value);
uint16_t *port_ds_word_ptr(uint16_t off);

/* ---- struct hosts (port/core/hosts.c) ------------------------------------- */

void *port_textout_host(void);   /* TextHost {event, sp}: sp = port_sp */
uint16_t *port_textout_host_sp(void);  /* its sp (svc_textout saves/restores it) */
void *port_uiprim_host(void);    /* UiHost {memory, segments, event} */
void *port_dialogs_host(void);   /* DialogHost {event, load, scratch} */
void *port_numdlg_host(void);    /* numdlg's bridge {service} */

/* ---- far code pointers of the recovered data ------------------------------ */

typedef uint16_t (*PortCellPredicate)(uint8_t *ds, uint8_t x, uint8_t y, uint8_t side);
/* 145c:1114 / 145c:1152, the two predicates of map/cellpred.c. */
PortCellPredicate c2_cell_predicate(uint16_t off, uint16_t seg);
typedef uint8_t (*PortByteReader)(void);
/* 0a04:0132 / 0a04:01fc, UIPRIM's picture stream readers. */
PortByteReader port_picture_reader(uint16_t off, uint16_t seg);
/* 1707:03e4 daily_show(id, side, far presenter (offset, segment)). */
void port_daily_show(uint8_t *ds, uint16_t id, uint16_t side, uint16_t off, uint16_t seg);
/* A briefing of core/scenbrf.c with its 100-byte closing buffer reserved. */
typedef void (*PortBriefing)(uint8_t *ds, uint16_t (*event)(uint8_t *, uint16_t, const uint16_t *, uint16_t),
                             uint16_t closing);
uint32_t port_run_briefing(uint8_t *ds, PortBriefing briefing);
/* Address-keyed forward with reordered words: out[i] = in[order[i]]. */
uint16_t btl_reorder(uint8_t *ds, uint32_t address, const uint16_t *args, unsigned count,
                     const unsigned char *order, unsigned n);
/* Call a linked routine by its reference name. */
uint32_t port_call_named(uint8_t *ds, const char *name, const uint16_t *args, unsigned count);

/* ---- driver register blocks (int86 REGS words: ax bx cx dx si di cflag) ---- */

uint16_t port_mouse_int33(uint16_t *w);
uint16_t port_sound_int66(uint16_t *w, uint16_t *ds_seg);

/* ---- keyboard prompts with their 16-byte scratch area ---------------------- */

uint16_t port_prompt_cancel(uint8_t *ds, uint16_t text, uint16_t max, uint16_t items);
uint16_t port_prompt_required(uint8_t *ds, uint16_t text, uint16_t max, uint16_t items);
uint16_t port_prompt_yes_no(uint8_t *ds, uint16_t text);

/* ---- LOADSCEN: read SCENARIO and MAP by name, hand over their bytes -------- */

void port_load_scenario(uint8_t *ds, uint16_t scenario);

/* ---- boundary pointers of the recovered code ------------------------------ */

void port_install_hooks(void);

#ifdef __cplusplus
}
#endif
#endif
