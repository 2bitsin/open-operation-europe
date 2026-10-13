// The game's link glue (game/link/glue.cpp and hosts.cpp): what a service needs beyond a direct call, a platform
// difference of the 16-bit original rather than game logic:
//
//  - the INT 33h / INT 66h register blocks of the mouse and FM drivers;
//  - LOADSCEN's file contents (it takes the two files' bytes).
#pragma once

#include "common/events.hpp"

#include "port.hpp"

#include <cstddef>
#include <cstdint>

/* ---- DS words --------------------------------------------------------------- */

/* The word at a near address, for a callee that writes its result through a word pointer. */
auto port_ds_word_ptr(Game& game, uint16_t near) -> uint16_t*;

/* ---- driver register blocks (int86 REGS words: ax bx cx dx si di cflag) ---- */

auto port_mouse_int33(uint16_t* w) -> uint16_t;
auto port_sound_int66(uint16_t* w, uint16_t* ds_seg) -> uint16_t;

/* ---- LOADSCEN: read SCENARIO and MAP by name, hand over their bytes -------- */

void port_load_scenario(Game& game, uint16_t scenario);
