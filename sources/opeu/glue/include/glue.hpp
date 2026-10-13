/* Platform glue between the game's services and the port layer.
 *
 * game/ resolves the numbered services of every recovered module to direct
 * calls (tools/game-services.json, tools/make_game_tree.py stage 11). What a
 * service needs beyond "call the routine with converted arguments" is not
 * game logic but a platform difference of the 16-bit original, and lives
 * here (port/core/glue.c, port/core/hosts.c):
 *
 *  - the C runtime random state, kept at DS:c6f4 by the original and in
 *    opeu_random_state by the recovered generator;
 *  - the struct hosts of TEXTOUT, UIPRIM and DIALOGS (stack pointer, memory
 *    and segment bases the host branches read) and NUMDLG's bridge;
 *  - far code pointers read from the recovered data (cell predicates,
 *    picture stream readers), mapped to the C functions they name;
 *  - the INT 33h / INT 66h register blocks of the mouse and FM drivers;
 *  - LOADSCEN's file contents (it takes the two files' bytes, the original
 *    read them by name).
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "port.hpp"

/* ---- C runtime random state (DS:c6f4) ------------------------------------- */

auto port_rand_bounded(uint8_t* ds, uint16_t bound) -> uint16_t;

/* ---- DS words --------------------------------------------------------------- */

void port_set_word(uint8_t* ds, uint16_t at, uint16_t value);
auto port_ds_word_ptr(uint16_t off) -> uint16_t*;

/* ---- struct hosts (port/core/hosts.c) ------------------------------------- */

auto port_textout_host() -> void*; /* TextHost {event} */
auto port_uiprim_host() -> void*;  /* UiHost {memory, segments, event} */
auto port_dialogs_host() -> void*; /* DialogHost {event, load, scratch} */
auto port_numdlg_host() -> void*;  /* numdlg's bridge {service} */

/* ---- far code pointers of the recovered data ------------------------------ */

using PortCellPredicate = uint16_t (*)(uint8_t* ds, uint8_t x, uint8_t y, uint8_t side);
/* 145c:1114 / 145c:1152, the two predicates of map/cellpred.c. */
auto c2_cell_predicate(uint16_t off, uint16_t seg) -> PortCellPredicate;
using PortByteReader = uint8_t (*)();
/* 0a04:0132 / 0a04:01fc, UIPRIM's picture stream readers. */
auto port_picture_reader(uint16_t off, uint16_t seg) -> PortByteReader;
/* 1707:03e4 daily_show(id, side, far presenter (offset, segment)). */
void port_daily_show(uint8_t* ds, uint16_t id, uint16_t side, uint16_t off, uint16_t seg);
/* Address-keyed forward with reordered words: out[i] = in[order[i]]. */
auto btl_reorder(uint8_t* ds, uint32_t address, const uint16_t* args, unsigned count, const unsigned char* order, unsigned n) -> uint16_t;
/* Call a linked routine by its reference name. */
auto port_call_named(uint8_t* ds, const char* name, const uint16_t* args, unsigned count) -> uint32_t;

/* ---- driver register blocks (int86 REGS words: ax bx cx dx si di cflag) ---- */

auto port_mouse_int33(uint16_t* w) -> uint16_t;
auto port_sound_int66(uint16_t* w, uint16_t* ds_seg) -> uint16_t;

/* ---- keyboard prompts with their 16-byte scratch area ---------------------- */

auto port_prompt_cancel(uint8_t* ds, uint16_t text, uint16_t max, uint16_t items) -> uint16_t;
auto port_prompt_required(uint8_t* ds, uint16_t text, uint16_t max, uint16_t items) -> uint16_t;
auto port_prompt_yes_no(uint8_t* ds, uint16_t text) -> uint16_t;

/* ---- LOADSCEN: read SCENARIO and MAP by name, hand over their bytes -------- */

void port_load_scenario(uint8_t* ds, uint16_t scenario);

/* ---- boundary pointers of the recovered code ------------------------------ */

void port_install_hooks();
