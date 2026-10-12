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
#pragma once

#include <stddef.h>
#include <stdint.h>

/* ---- lifecycle ---------------------------------------------------------- */

struct PlatConfig {
  const char* title;      /* window title */
  int width, height;      /* logical framebuffer size (640x480 for mode 12h) */
  int scale;              /* initial window scale factor (>=1) */
  int headless;           /* 1: no window, no audio device, no real input */
  int fullscreen;
};

auto plat_init(const PlatConfig* cfg) -> int;   /* 0 on success, -1 on failure */
void plat_shutdown();

/* ---- video -------------------------------------------------------------- */

/* Present one frame.  pixels: width*height palette indices, row-major,
 * pitch == width.  palette: 256 entries of 8-bit R,G,B.  Called at the
 * emulated display rate (about 60-70 Hz); in headless mode it only records
 * the frame so plat_capture() can return it. */
void plat_present(const uint8_t* pixels, int width, int height, const uint8_t* palette);

/* Write the last presented frame as a PNG file.  Returns 0 on success. */
auto plat_write_png(const char* path) -> int;

/* ---- input -------------------------------------------------------------- */

// NOLINTNEXTLINE(cppcoreguidelines-use-enum-class): PlatEvent::type holds these as ints across the contract
enum {
  PLAT_EV_NONE = 0,
  PLAT_EV_QUIT,           /* window closed / Cmd-Q */
  PLAT_EV_KEY_DOWN,       /* scancode + ascii valid */
  PLAT_EV_KEY_UP,         /* scancode valid */
  PLAT_EV_MOUSE_MOVE,     /* x, y valid (framebuffer pixels, clamped) */
  PLAT_EV_MOUSE_DOWN,     /* button valid (0 left, 1 right, 2 middle), x, y */
  PLAT_EV_MOUSE_UP
};

struct PlatEvent {
  int type;
  uint8_t scancode;       /* PC set-1 make code; bit 7 clear.  0xE0-prefixed
                               keys (arrows, Home, ...) use their base code and
                               set extended=1 */
  uint8_t extended;
  uint8_t ascii;          /* BIOS INT 16h ASCII for this key with current
                               modifiers (0 for none) */
  uint8_t button;
  int x, y;
};

/* Pump the OS event queue and return the next event; 0 when empty. */
auto plat_poll_event(PlatEvent* ev) -> int;

/* Current modifier state as BIOS shift flags (0040:0017 layout):
 * bit0 right shift, bit1 left shift, bit2 ctrl, bit3 alt. */
auto plat_shift_flags() -> uint8_t;

/* ---- time --------------------------------------------------------------- */

auto plat_ticks_us() -> uint64_t;   /* monotonic microseconds since plat_init */
void plat_sleep_ms(int ms); /* yields; never called on Emscripten frames */

/* ---- files -------------------------------------------------------------- */

/* The save directory: the only place the game's files touch the file
 * system (the shipped data files are compiled into the executable and
 * served by services/fileio.c).  Default: SDL's per-user pref path
 * natively, /saves (the page's IDBFS mount) in the browser.  Paths given to
 * plat_fopen are DOS names relative to it ("SAVEDATA"); lookup is
 * case-insensitive.  mode is a C stdio mode string.  Returns NULL if
 * missing.  The handle is a stdio FILE* on every current backend. */
void plat_set_save_dir(const char* dir);
auto plat_save_dir() -> const char*;
auto plat_fopen(const char* dos_name, const char* mode) -> void*;
auto plat_fclose(void* f) -> int;
auto plat_fread(void* buf, size_t n, void* f) -> size_t;
auto plat_fwrite(const void* buf, size_t n, void* f) -> size_t;
auto plat_fseek(void* f, long off, int whence) -> long;   /* returns new position or -1 */
auto plat_fsize(void* f) -> long;
auto plat_fexists(const char* dos_name) -> int;
auto plat_fdelete(const char* dos_name) -> int;
auto plat_frename(const char* from, const char* to) -> int;

/* ---- audio -------------------------------------------------------------- */

/* The audio callback fills `frames` stereo int16 frames at `rate` Hz.
 * It runs on the audio thread; it must never block.  A NULL callback (or
 * headless mode) means silence. */
using PlatAudioFn = void (*)(void* user, int16_t* out, int frames, int rate);
auto plat_audio_open(int rate, PlatAudioFn fn, void* user) -> int;
auto plat_audio_active() -> int;           /* 1 when a device stream is open */
void plat_audio_lock();
void plat_audio_unlock();
