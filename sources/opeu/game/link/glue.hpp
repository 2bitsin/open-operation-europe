// The game's link glue (game/link/glue.cpp and hosts.cpp): what a service needs beyond "call the routine with
// converted arguments", a platform difference of the 16-bit original rather than game logic:
//
//  - the C runtime random state, kept at DS:c6f4 by the saved game and in opeu_random_state by the generator;
//  - the struct hosts of TEXTOUT, UIPRIM and DIALOGS (the memory and segment bases their routines read) and NUMDLG's
//    bridge;
//  - far code pointers read from the data (cell predicates, picture stream readers), mapped to the functions they name;
//  - the INT 33h / INT 66h register blocks of the mouse and FM drivers;
//  - LOADSCEN's file contents (it takes the two files' bytes).
#pragma once

#include "common/events.hpp"

#include "port.hpp"

#include <cstddef>
#include <cstdint>

/* ---- C runtime random state (DS:c6f4) ------------------------------------- */

auto port_rand_bounded(Game& game, uint16_t bound) -> uint16_t;

/* ---- DS words --------------------------------------------------------------- */

/* The word at a near address, for a callee that writes its result through a word pointer. */
auto port_ds_word_ptr(Game& game, uint16_t near) -> uint16_t*;

/* ---- struct hosts (port/core/hosts.c) ------------------------------------- */

auto port_textout_host() -> void*; /* TextHost {event} */
auto port_uiprim_host() -> void*;  /* UiHost {memory, segments, event} */
auto port_dialogs_host() -> void*; /* DialogHost {event, load, scratch} */
auto port_numdlg_host() -> void*;  /* numdlg's bridge {service} */

/* ---- far code pointers of the recovered data ------------------------------ */

using PortCellPredicate = uint16_t (*)(Game& game, uint8_t x, uint8_t y, uint8_t side);
/* 145c:1114 / 145c:1152, the two predicates of map/cellpred.c. */
auto c2_cell_predicate(uint16_t off, uint16_t seg) -> PortCellPredicate;
using PortByteReader = uint8_t (*)();
/* 0a04:0132 / 0a04:01fc, UIPRIM's picture stream readers. */
auto port_picture_reader(uint16_t off, uint16_t seg) -> PortByteReader;
/* 1707:03e4 daily_show(id, side, far presenter (offset, segment)). */
void port_daily_show(Game& game, uint16_t id, uint16_t side, uint16_t off, uint16_t seg);
/* Call a linked routine by its reference name. */
auto port_call_named(Game& game, const char* name, const uint16_t* args, unsigned count) -> uint32_t;

/* ---- driver register blocks (int86 REGS words: ax bx cx dx si di cflag) ---- */

auto port_mouse_int33(uint16_t* w) -> uint16_t;
auto port_sound_int66(uint16_t* w, uint16_t* ds_seg) -> uint16_t;

/* ---- keyboard prompts with their 16-byte scratch area ---------------------- */

auto port_prompt_cancel(Game& game, uint16_t text, uint16_t max, uint16_t items) -> uint16_t;
auto port_prompt_required(Game& game, uint16_t text, uint16_t max, uint16_t items) -> uint16_t;
auto port_prompt_yes_no(Game& game, uint16_t text) -> uint16_t;

/* ---- LOADSCEN: read SCENARIO and MAP by name, hand over their bytes -------- */

void port_load_scenario(Game& game, uint16_t scenario);

/* ---- boundary pointers of the recovered code ------------------------------ */

void port_install_hooks();
