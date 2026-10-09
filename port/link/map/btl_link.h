/*
 * src/main/map btl*.c, dpl*.c, hqloss.c modules and core/stockord.c, plus the
 * PORT_ROUTINE handlers they reach under port/link/routines/).
 * Conversion only; no game logic.
 *
 * Address-keyed word convention of the handlers (port.h): the register words
 * in AX, DX, BX order (bytes zero-extended), then the stack words in
 * prototype order; for the MSC C 6.0a _fastcall callees that is the order in
 * which the arguments take registers: left to right, a near pointer goes to
 * BX when it is free, any other byte/word (and a pointer when BX is taken) to
 * the first free of AX, DX, BX; far pointers, longs and the arguments left
 * over go to the stack (cdecl/asm routines: all stack words in order). */
#ifndef PORT_LINK_MAP_BTL_LINK_H
#define PORT_LINK_MAP_BTL_LINK_H

#include "link.h"

/* Modeled DGROUP stack for a callee entered by a far call whose host entry
 * takes `sp` (its entry SP, the far return address slot) or `frame` (its BP,
 * entry SP - 2). The reserve covers the callee's locals and the frames of
 * the routines its host branch calls directly at sp/frame - n (the deepest
 * documented battle buffers and nested frames stay below 0x180 bytes). */
#define BTL_RESERVE 0x200u

static inline uint16_t btl_sp_enter(void)
{
    return port_stack_enter(BTL_RESERVE);
}

static inline void btl_sp_leave(void)
{
    port_stack_leave(BTL_RESERVE);
}

#define BTL_BP(sp) ((uint16_t)((sp) - 2u))

/* A scratch buffer of `bytes` free DS bytes on the modeled stack. */
static inline uint16_t btl_scratch_enter(uint16_t bytes)
{
    port_stack_enter(bytes);
    return (uint16_t)(port_sp + 4u);
}

static inline void btl_scratch_leave(uint16_t bytes)
{
    port_stack_leave(bytes);
}

/* The cdecl variadic text routines of textout (0480:0b9a request_format,
 * 0480:0bb8 cursor_print/army_print) read their pushed argument words at
 * the DS offset `frame` (textout.c REQUEST_PARAMS / PRINT_PARAMS). The words
 * are laid out on the modeled stack as the original pushed them (first
 * argument at the lowest address). */
typedef void (*BtlFrameRoutine)(uint8_t *, void *, unsigned short);

static inline void btl_frame_call(uint8_t *ds, BtlFrameRoutine routine, const uint16_t *words, unsigned n)
{
    uint16_t bytes = (uint16_t)(2u * n + 2u), frame;
    unsigned i;
    port_stack_enter(bytes);
    frame = (uint16_t)(port_sp + 4u);
    for (i = 0; i < n; ++i) {
        ds[(uint16_t)(frame + 2u * i)] = (uint8_t)words[i];
        ds[(uint16_t)(frame + 2u * i + 1u)] = (uint8_t)(words[i] >> 8);
    }
    routine(ds, link_textout_host(), frame);
    port_stack_leave(bytes);
}

/* Forward an address-keyed service with its words reordered:
 * out[i] = in[order[i]] (an order entry past `count` gives 0). */
static inline uint16_t btl_reorder(uint8_t *ds, uint32_t address, const uint16_t *args, unsigned count,
                                   const unsigned char *order, unsigned n)
{
    uint16_t out[8];
    unsigned i;
    for (i = 0; i < n && i < 8; ++i)
        out[i] = order[i] < count ? args[order[i]] : 0;
    return (uint16_t)port_call(ds, address, out, n);
}

/* ---- per-domain conversions -------------------------------------------- *
 * Battle host branches call their sibling modules directly and hand them the
 * callback they received, so the services of every module reached that way
 * arrive at the callback of the module the routine handler entered. The
 * modules below pack some services differently from the handler convention;
 * each group is closed under those direct calls, and its link callbacks all
 * use the group's conversion. */

/* Deployment (dplauto, dplcell, dplrows, dplside, dplunit): words in source
 * (prototype) order, tests/validate_deployment.py. */
static inline uint16_t btl_deploy_event(uint8_t *ds, uint32_t address, const uint16_t *args, unsigned count)
{
    static const unsigned char unit_name[] = {1, 0};          /* (unit, buffer) */
    static const unsigned char list_dialog[] = {1, 2, 0};     /* (window, limit, flags) */
    static const unsigned char set_order[] = {1, 0};          /* (element, order) */
    static const unsigned char grid_step[] = {1, 2, 0};       /* (x*, y*, direction) */
    static const unsigned char put_range[] = {1, 2, 0};       /* (s, start, count) */
    static const unsigned char columns[] = {0, 2, 1, 3, 4};   /* (column, first, rows, second, third) */
    static const unsigned char message[] = {1, 2, 0};         /* (text, side, value) */
    switch (address) {
    case 0x0b8b8u: return btl_reorder(ds, address, args, count, unit_name, 2);
    case 0x0b4a0u: return btl_reorder(ds, address, args, count, list_dialog, 3);
    case 0x29d7au: return btl_reorder(ds, address, args, count, set_order, 2);
    case 0x152feu: return btl_reorder(ds, address, args, count, grid_step, 3);
    case 0x053d0u: return btl_reorder(ds, address, args, count, put_range, 3);
    case 0x0aa20u: return btl_reorder(ds, address, args, count, columns, 5);
    case 0x2a2b2u: return btl_reorder(ds, address, args, count, message, 3);
    default: return port_service(ds, address, args, count);
    }
}

/* BTLCMDR and BTLLOSS (element_destroy is called natively from btlloss):
 * two-pointer services pass BX before AX (validate_commander_names.py). */
static inline uint16_t btl_commander_event(uint8_t *ds, uint32_t address, const uint16_t *args, unsigned count)
{
    static const unsigned char swap[] = {1, 0};
    switch (address) {
    case 0x09e90u:  /* string_copy (BX destination, AX source) */
    case 0x2990eu:  /* remove_unit (BX unit, AX element) */
    case 0x2e828u:  /* 2db6:0cc8 ranged_fire (BX attacker, AX target) */
        return btl_reorder(ds, address, args, count, swap, 2);
    default: return port_service(ds, address, args, count);
    }
}

/* Player orders segment 2ec8 (btlcmds, btlcurs, btlexec, btlexit, btlinfo,
 * btlmenu, btlplay, btlreach, btlsel): btlcurs reads byte i of the hex mask
 * table, FAR_DATA segment 352d, through the identifier 0x352d0 + i
 * (btlcurs.c host branch; the table is 3 x 130 bytes). */
#define BTL_HEX_MASKS 0x352d0u
static inline uint16_t btl_orders_event(uint8_t *ds, uint32_t address, const uint16_t *args, unsigned count)
{
    if (address >= BTL_HEX_MASKS && address < BTL_HEX_MASKS + 390u)
        return *port_far(0x352du, (uint16_t)(address - BTL_HEX_MASKS));
    return port_service(ds, address, args, count);
}

/* BTLMINES (MineEvent): words in prototype order (btlmines.c host branch,
 * validate_battle_mines.py). */
static inline uint16_t btl_mines_event(uint8_t *ds, uint32_t address, const uint16_t *args, unsigned count)
{
    static const unsigned char set_order[] = {1, 0};   /* (element, order) */
    static const unsigned char report[] = {1, 2, 0};   /* (text, side, value) */
    switch (address) {
    case 0x29d7au: return btl_reorder(ds, address, args, count, set_order, 2);
    case 0x2a284u: return btl_reorder(ds, address, args, count, report, 3);
    default: return port_service(ds, address, args, count);
    }
}

/* Battle AI segment 2c49 (btlaiact, btlaient, btlaimov, btlaipwr, btlaitrn,
 * btlaiunt): path_step {x, y} carries the VALUES of the two frame bytes whose
 * addresses the original passes (it never reads them back). The handler
 * takes the pointers (AX y, BX x): the values go to two modeled stack bytes
 * that stand for the original's frame bytes. */
static inline uint16_t btl_ai_event(uint8_t *ds, uint32_t address, const uint16_t *args, unsigned count)
{
    if (address == 0x2a528u) {
        uint16_t cell = btl_scratch_enter(2), words[2], r;
        ds[cell] = (uint8_t)(count > 0 ? args[0] : 0);
        ds[(uint16_t)(cell + 1u)] = (uint8_t)(count > 1 ? args[1] : 0);
        words[0] = (uint16_t)(cell + 1u);
        words[1] = cell;
        r = (uint16_t)port_call(ds, address, words, 2);
        btl_scratch_leave(2);
        return r;
    }
    return port_service(ds, address, args, count);
}

#endif
