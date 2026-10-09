/* Host entries of the recovered OPEN.EXE modules (src/open), as their host
 * branches declare them, for the OPEN link layer (port/open/link).
 *
 * Every name is the recovered one; open_names.h (included first) maps it to
 * the link name the forced include gives the src/open objects (open_ +
 * name), so these prototypes and the compiled modules agree.
 *
 * Each recovered module calls everything outside its own code segment
 * through one service callback, with the module's own callback type
 * (written out here exactly as the module declares it):
 *   OPENMAIN 03ac  OpenEvent    (ds, id, args, count) -> AX
 *   OUTIL    062d  UtilService  (ds, id, words, count) -> DX:AX
 *   OTEXT    02a0  TextEvent    via a TextHost {event, sp}
 *   OEMSMAP  032a  EmsEvent     (ds, id, args, count) -> AX
 *   OSOUND   0349  SoundService (ds, call, words) -> AX
 */
#ifndef PORT_OPEN_RECOVERED_H
#define PORT_OPEN_RECOVERED_H

#include <stdint.h>
#include "open_names.h"

typedef uint16_t (*OpenEvent)(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count);
typedef uint32_t (*UtilService)(uint8_t *ds, uint16_t id, uint16_t *words, uint16_t count);
typedef uint16_t (*TextEvent)(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count);
typedef struct { TextEvent event; uint16_t sp; } TextHost;    /* otext.c host structure */
typedef uint16_t (*EmsEvent)(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count);
typedef uint16_t (*SoundService)(uint8_t *ds, uint16_t call, uint16_t *words);

/* ---- OPENMAIN (src/open/core/openmain.c, 03ac) --------------------------- */
void opeu_open_main(uint8_t *ds, OpenEvent event);                       /* 03ac:278a, main() */
void opeu_open_delay(uint8_t *ds, OpenEvent event, uint16_t ticks);      /* 03ac:0004 */

/* ---- OUTIL (src/open/core/outil.c, 062d) --------------------------------- */
uint16_t opeu_mouse_reset(uint8_t ds[65536], UtilService event);
uint8_t opeu_stream_file_byte(uint8_t ds[65536], UtilService event);
void opeu_picture_file_at(uint8_t ds[65536], UtilService event, uint16_t x, uint16_t y, int32_t position);
void opeu_open_palette_set(uint8_t ds[65536], UtilService event, uint8_t *palette);
int16_t opeu_open_palette_step(uint8_t ds[65536], UtilService event, uint8_t *target);
int16_t opeu_open_palette_fade(uint8_t ds[65536], UtilService event, uint8_t *target, uint16_t mode);
int16_t opeu_open_fade_in(uint8_t ds[65536], UtilService event);
int16_t opeu_open_fade_in_wait(uint8_t ds[65536], UtilService event);
int16_t opeu_open_fade_alt(uint8_t ds[65536], UtilService event);
void opeu_open_identity_palette(uint8_t ds[65536], UtilService event);
int16_t opeu_open_input_poll(uint8_t ds[65536], UtilService event);
int16_t opeu_open_file_or_die(uint8_t ds[65536], UtilService event, uint16_t name);
void opeu_open_stub_a(uint8_t ds[65536], UtilService event);
void opeu_open_stub_b(uint8_t ds[65536], UtilService event);
void opeu_open_stub_c(uint8_t ds[65536], UtilService event);

/* ---- OTEXT (src/open/core/otext.c, 02a0) --------------------------------- */
void opeu_input_flush(uint8_t ds[65536], TextHost *host);
uint8_t opeu_text_put(uint8_t ds[65536], TextHost *host, uint8_t c);
void opeu_ui_text(uint8_t ds[65536], TextHost *host, uint16_t s);
void opeu_cursor_print(uint8_t ds[65536], TextHost *host, uint16_t frame);

/* ---- OEMSMAP (src/open/core/oemsmap.c, 032a) ----------------------------- */
void opeu_gfx_wait(uint8_t ds[65536], EmsEvent event);
int16_t opeu_memory_release(uint8_t ds[65536], EmsEvent event);
int16_t opeu_ems_alloc_sprite(uint8_t ds[65536], EmsEvent event, uint16_t sp);
int16_t opeu_ems_detect(uint8_t ds[65536], EmsEvent event, uint16_t sp);

/* ---- OSOUND (src/open/io/osound.c, 0349) --------------------------------- */
void opeu_sound_initialize(uint8_t ds[65536], SoundService service);
void opeu_finish_side_initialization(uint8_t ds[65536], SoundService service, uint8_t value);
int opeu_music_playing(uint8_t ds[65536], SoundService service);
void opeu_music_start(uint8_t ds[65536], SoundService service, uint8_t track);

#endif
