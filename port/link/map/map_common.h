/*
 * map view, scrolling, cursor, unit drawing, movement and order targets).
 * Conversion only; no game logic. */
#ifndef PORT_LINK_MAP_COMMON_H
#define PORT_LINK_MAP_COMMON_H

#include <string.h>
#include "link.h"

/* `frame` parameter of the map host branches ("Host frame convention" in
 * src/main/map/mapscrol.c and siblings): the callee's original BP, or its
 * entry SP minus 2 when it has none -- the entry SP - 2 of a far call in
 * both cases. The recovered code derives its nested callee frames as
 * frame-INNER without touching port_sp (mapscrol: scroll_to -> scroll ->
 * cell_draw -> unit_draw reaches about 0x150 bytes below the first frame,
 * the unit draw's 216-byte sprite buffer included), so the reserve covers
 * the deepest such chain. */
#define MAP_FRAME_BYTES 0x300u

static inline uint16_t map_frame_enter(void)
{
    return (uint16_t)(port_stack_enter(MAP_FRAME_BYTES) - 2u);
}

static inline void map_frame_leave(void)
{
    port_stack_leave(MAP_FRAME_BYTES);
}

/* The cdecl variadic text routines of TEXTOUT (0480:0b9a request_format =
 * sprintf, 0480:0bb8 cursor_print) read the pushed argument words at DS
 * `frame` (textout.c REQUEST_PARAMS / PRINT_PARAMS): the first argument word
 * at `frame`, as the original call left them above the return address. */
typedef void (*MapFrameRoutine)(uint8_t *, void *, unsigned short);

static inline void map_text_call(uint8_t *ds, MapFrameRoutine routine, const uint16_t *words, unsigned n)
{
    uint16_t bytes = (uint16_t)(2u * n), frame;
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

/* 0480:0b9a request_format(destination, format, ...) */
static inline void map_request_format(uint8_t *ds, const uint16_t *words, unsigned n)
{
    map_text_call(ds, opeu_request_format, words, n);
}

/* 0480:0bb8 cursor_print(format, ...) */
static inline void map_cursor_print(uint8_t *ds, const uint16_t *words, unsigned n)
{
    map_text_call(ds, opeu_cursor_print, words, n);
}

/* 09f6:00d4 rand_bounded. The host branches keep the generator state in
 * DS:c6f4 (they copy it into opeu_random_state around each call); a service
 * that calls the routine for them does the same. */
extern uint32_t opeu_random_state;
static inline uint16_t map_rand_bounded(uint8_t *ds, uint16_t bound)
{
    uint16_t result;
    memcpy(&opeu_random_state, ds + 0xc6f4, 4);
    result = opeu_rand_bounded(bound);
    memcpy(ds + 0xc6f4, &opeu_random_state, 4);
    return result;
}

/* 1707:03e4 daily_show(id, side, far presenter (offset, segment)); DIALOGS
 * calls the presenter back by routine identifier (seg << 16 | off). */
static inline void map_daily_show(uint8_t *ds, uint16_t id, uint16_t side, uint16_t off, uint16_t seg)
{
    opeu_daily_show(ds, link_dialogs_host(), id, (uint8_t)side, (uint32_t)seg << 16 | off);
}

/* FAR_DATA segment 4582: the 40x40 movement path-cost grid at 4582:0c80
 * (movecost.c GRID_ARG, toward.c far_data). */
#define MAP_PATH_SEGMENT 0x4582u

/* The 15c4 map segment's shared service kinds ("PLAN kinds" / "global map
 * kinds" in the host branches of MAPCELL, MAPFRAME, MAPVIEW, MAPSCROL,
 * MAPERASE, MAPOVER, MAPUNIT; MAPPOS forwards its event to MAPSCROL and
 * MAPERASE). The modules hand their event down to the 15c4 routines they
 * call directly, so every 15c4 module callback serves the whole set. */
uint16_t map_plan_service(uint8_t *ds, const char *module, uint16_t kind, const uint16_t *args, uint16_t count);

/* The movement source file's shared service ids (map_move.c), served for
 * MOVEUNIT and MOVETGT. */
uint16_t map_move_service(uint8_t *ds, const char *module, uint16_t id, const uint16_t *args, uint16_t count);

#endif
