// The game's link glue (game/link/glue.cpp and hosts.cpp): what a service needs beyond a direct call, a platform
// difference of the 16-bit original rather than game logic:
//
//  - the C runtime random state, kept at DS:c6f4 by the saved game and in opeu_random_state by the generator;
//  - the struct hosts of TEXTOUT, UIPRIM and DIALOGS (the memory and segment bases their routines read) and NUMDLG's
//    bridge;
//  - the daily show with the dialogs' host (the presenter is a Presenter enumerator, core/presenters runs it);
//  - the INT 33h / INT 66h register blocks of the mouse and FM drivers;
//  - LOADSCEN's file contents (it takes the two files' bytes).
#pragma once

#include "common/events.hpp"
#include "common/presenter.hpp"

#include "port.hpp"

#include <cstddef>
#include <cstdint>

/* ---- C runtime random state (DS:c6f4) ------------------------------------- */

auto port_rand_bounded(Game& game, uint16_t bound) -> uint16_t;

/* ---- DS words --------------------------------------------------------------- */

/* The word at a near address, for a callee that writes its result through a word pointer. */
auto port_ds_word_ptr(Game& game, uint16_t near) -> uint16_t*;

/* ---- struct hosts (port/core/hosts.c) ------------------------------------- */

auto port_textout_host() -> void*; /* TextHost {} */
auto port_uiprim_host() -> void*;  /* UiHost {memory, segments, event} */
auto port_dialogs_host() -> void*; /* DialogHost {load, scratch} */

/* ---- the daily show ----------------------------------------------------- */

/* daily_show(id, side, presenter) with the dialogs' host. */
void port_daily_show(Game& game, uint16_t id, uint16_t side, opeu::game::Presenter presenter);

/* ---- driver register blocks (int86 REGS words: ax bx cx dx si di cflag) ---- */

auto port_mouse_int33(uint16_t* w) -> uint16_t;
auto port_sound_int66(uint16_t* w, uint16_t* ds_seg) -> uint16_t;

/* ---- keyboard prompts with their 16-byte scratch area ---------------------- */

auto port_prompt_cancel(Game& game, uint16_t text, uint16_t max, uint16_t items) -> uint16_t;
auto port_prompt_required(Game& game, uint16_t text, uint16_t max, uint16_t items) -> uint16_t;
auto port_prompt_yes_no(Game& game, uint16_t text) -> uint16_t;

/* ---- LOADSCEN: read SCENARIO and MAP by name, hand over their bytes -------- */

void port_load_scenario(Game& game, uint16_t scenario);
