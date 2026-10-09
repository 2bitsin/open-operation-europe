/* Platform contract for the Operation Europe port.
 *
 * Everything the game needs from the host machine goes through this header:
 * video (an 8-bit indexed framebuffer plus a 256-entry RGB palette), input
 * (PC/XT set-1 scancodes with ASCII, absolute mouse in game pixels), time,
 * files (the original game directory), and audio (a pull callback that an
 * OPL emulator will feed).  The SDL3 implementation lives in
 * port/platform/sdl3/; a future Emscripten build uses the same backend.
 *
 * Rules: plain C99, no blocking calls except plat_sleep_ms(), no global
 * state visible outside the backend, every function callable when the
 * backend runs headless (no window, no audio device).
 */
#ifndef OPEU_PLATFORM_H
#define OPEU_PLATFORM_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- lifecycle ---------------------------------------------------------- */

typedef struct PlatConfig {
    const char *title;      /* window title */
    int width, height;      /* logical framebuffer size (640x480 for mode 12h) */
    int scale;              /* initial window scale factor (>=1) */
    int headless;           /* 1: no window, no audio device, no real input */
    int fullscreen;
} PlatConfig;

int  plat_init(const PlatConfig *cfg);   /* 0 on success, -1 on failure */
void plat_shutdown(void);

/* ---- video -------------------------------------------------------------- */

/* Present one frame.  pixels: width*height palette indices, row-major,
 * pitch == width.  palette: 256 entries of 8-bit R,G,B.  Called at the
 * emulated display rate (about 60-70 Hz); in headless mode it only records
 * the frame so plat_capture() can return it. */
void plat_present(const uint8_t *pixels, int width, int height,
                  const uint8_t palette[256 * 3]);

/* Write the last presented frame as a PNG file.  Returns 0 on success. */
int plat_write_png(const char *path);

/* ---- input -------------------------------------------------------------- */

enum {
    PLAT_EV_NONE = 0,
    PLAT_EV_QUIT,           /* window closed / Cmd-Q */
    PLAT_EV_KEY_DOWN,       /* scancode + ascii valid */
    PLAT_EV_KEY_UP,         /* scancode valid */
    PLAT_EV_MOUSE_MOVE,     /* x, y valid (framebuffer pixels, clamped) */
    PLAT_EV_MOUSE_DOWN,     /* button valid (0 left, 1 right, 2 middle), x, y */
    PLAT_EV_MOUSE_UP
};

typedef struct PlatEvent {
    int type;
    uint8_t scancode;       /* PC set-1 make code; bit 7 clear.  0xE0-prefixed
                               keys (arrows, Home, ...) use their base code and
                               set extended=1 */
    uint8_t extended;
    uint8_t ascii;          /* BIOS INT 16h ASCII for this key with current
                               modifiers (0 for none) */
    uint8_t button;
    int x, y;
} PlatEvent;

/* Pump the OS event queue and return the next event; 0 when empty. */
int plat_poll_event(PlatEvent *ev);

/* Current modifier state as BIOS shift flags (0040:0017 layout):
 * bit0 right shift, bit1 left shift, bit2 ctrl, bit3 alt. */
uint8_t plat_shift_flags(void);

/* ---- time --------------------------------------------------------------- */

uint64_t plat_ticks_us(void);   /* monotonic microseconds since plat_init */
void     plat_sleep_ms(int ms); /* yields; never called on Emscripten frames */

/* ---- files -------------------------------------------------------------- */

/* The game directory (binaries/ by default).  Paths given to plat_fopen are
 * DOS names relative to it ("SCEDATA", "OPEU.SAV"); lookup is
 * case-insensitive.  mode is a C stdio mode string.  Returns NULL if
 * missing.  The handle is a stdio FILE* on every current backend. */
void  plat_set_data_dir(const char *dir);
const char *plat_data_dir(void);
void *plat_fopen(const char *dos_name, const char *mode);
int   plat_fclose(void *f);
size_t plat_fread(void *buf, size_t n, void *f);
size_t plat_fwrite(const void *buf, size_t n, void *f);
long  plat_fseek(void *f, long off, int whence);   /* returns new position or -1 */
long  plat_fsize(void *f);
int   plat_fexists(const char *dos_name);
int   plat_fdelete(const char *dos_name);
int   plat_frename(const char *from, const char *to);

/* ---- audio -------------------------------------------------------------- */

/* The audio callback fills `frames` stereo int16 frames at `rate` Hz.
 * It runs on the audio thread; it must never block.  A NULL callback (or
 * headless mode) means silence. */
typedef void (*PlatAudioFn)(void *user, int16_t *out, int frames, int rate);
int  plat_audio_open(int rate, PlatAudioFn fn, void *user);
void plat_audio_lock(void);
void plat_audio_unlock(void);

#ifdef __cplusplus
}
#endif
#endif
