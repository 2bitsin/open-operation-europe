/* Host event pump, presentation pacing and the services' time base.
 *
 * Every polling service (key available/read, mouse state, FM countdown and
 * status) calls port_pump(). The game never waits on anything else, so this
 * is where host events enter the keyboard/mouse driver state and where the
 * VRAM model is shown (at most about 60 frames per second).
 *
 * Window mode: host time; when the pump runs many times in a row without a
 * host event it sleeps 1 ms now and then, so busy-waits do not take a whole
 * CPU. The window close request leaves the game (port_exit(0)).
 *
 * Headless mode (--headless): no window; every pump advances the virtual
 * clock by 1 ms. Input comes from the script given with --input; when the
 * virtual clock reaches --ms the screen is rendered, written to --png and
 * the program exits with 0.
 *
 * Input script format (one event per line, '#' starts a comment):
 *
 *   <ms> key <name>              press and release a key
 *   <ms> key <scan>:<ascii>      the same with BIOS codes in hex (1c:0d)
 *   <ms> text <characters>       one key per character (letters, digits,
 *                                space and the US punctuation)
 *   <ms> move <x> <y>            move the pointer (screen pixels, 0..639 x
 *                                0..479; the game band starts at y=40)
 *   <ms> click <x> <y> [left|right]  move there, press the button at <ms>
 *                                and release it 100 ms later
 *   <ms> press <x> <y> [left|right] / <ms> release <x> <y> [left|right]
 *
 * <ms> is the virtual time in milliseconds. Key names: enter, esc, space,
 * tab, bs, up, down, left, right, home, end, pgup, pgdn, ins, del,
 * f1..f10, a..z, 0..9 (top row), kp0..kp9 (keypad, Num Lock off: scan code
 * only, as the cursor keys; textout maps scan codes 47h..52h to digits).
 */
#include "hostinput.hpp"
#include "platform.hpp"
#include "port.hpp"
#include "services.hpp"

#include <ctype.h>
#if defined(__APPLE__) || defined(__linux__)
#include <execinfo.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- time base ----------------------------------------------------------- */

static uint64_t virtual_us;

uint64_t port_time_us(void) {
  return port_options.headless ? virtual_us : plat_ticks_us();
}

/* ---- presentation -------------------------------------------------------- */

static uint8_t  frame[640 * 480];
static uint8_t  palette[768];
static uint64_t last_present_us;
static int      presented_once;

static void present(int force) {
  int changed = vgalib_render(frame, palette);
  int pointer = port_input_pointer_dirty();
  if (!force && !changed && !pointer && presented_once) return;
  port_input_draw_pointer(frame, palette);
  plat_present(frame, 640, 480, palette);
  presented_once = 1;
}

/* ---- scripted input (headless) ------------------------------------------- */

typedef struct {
  uint32_t ms;
  int      kind; /* 0 key, 1 move, 2 press, 3 release */
  uint8_t  scan, ascii;
  int      x, y, button;
} ScriptEvent;

static ScriptEvent* script;
static unsigned     script_count, script_next, script_cap;
static int          script_loaded;

static void script_add(ScriptEvent e) {
  if (script_count == script_cap) {
    script_cap = script_cap ? script_cap * 2 : 64;
    script     = (ScriptEvent*)realloc(script, script_cap * sizeof *script);
    if (!script) port_fatal("out of memory for the input script");
  }
  script[script_count++] = e;
}

static const struct {
  const char* name;
  uint8_t     scan, ascii;
} key_names[] = {
  { "enter", 0x1c, 0x0d },
  { "esc",   0x01, 0x1b },
  { "space", 0x39, 0x20 },
  { "tab",   0x0f, 0x09 },
  { "bs",    0x0e, 0x08 },
  { "up",    0x48, 0    },
  { "down",  0x50, 0    },
  { "left",  0x4b, 0    },
  { "right", 0x4d, 0    },
  { "home",  0x47, 0    },
  { "end",   0x4f, 0    },
  { "pgup",  0x49, 0    },
  { "pgdn",  0x51, 0    },
  { "ins",   0x52, 0    },
  { "del",   0x53, 0    },
  { "f1",    0x3b, 0    },
  { "f2",    0x3c, 0    },
  { "f3",    0x3d, 0    },
  { "f4",    0x3e, 0    },
  { "f5",    0x3f, 0    },
  { "f6",    0x40, 0    },
  { "f7",    0x41, 0    },
  { "f8",    0x42, 0    },
  { "f9",    0x43, 0    },
  { "f10",   0x44, 0    },
  { "kp0",   0x52, 0    },
  { "kp1",   0x4f, 0    },
  { "kp2",   0x50, 0    },
  { "kp3",   0x51, 0    },
  { "kp4",   0x4b, 0    },
  { "kp5",   0x4c, 0    },
  { "kp6",   0x4d, 0    },
  { "kp7",   0x47, 0    },
  { "kp8",   0x48, 0    },
  { "kp9",   0x49, 0    },
};

/* US layout: scan codes of the printable characters (unshifted, shifted). */
static const char    row_chars[] = "1234567890-=qwertyuiop[]asdfghjkl;'`\\zxcvbnm,./";
static const char    row_shift[] = "!@#$%^&*()_+QWERTYUIOP{}ASDFGHJKL:\"~|ZXCVBNM<>?";
static const uint8_t row_scan[]  = {
  0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b,
  0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
};

static int char_key(char c, uint8_t* scan, uint8_t* ascii) {
  const char* p;
  if (c == ' ') {
    *scan  = 0x39;
    *ascii = ' ';
    return 1;
  }
  if ((p = strchr(row_chars, c)) != NULL && c) {
    *scan  = row_scan[p - row_chars];
    *ascii = (uint8_t)c;
    return 1;
  }
  if ((p = strchr(row_shift, c)) != NULL && c) {
    *scan  = row_scan[p - row_shift];
    *ascii = (uint8_t)c;
    return 1;
  }
  return 0;
}

static int parse_key(const char* word, uint8_t* scan, uint8_t* ascii) {
  unsigned i, s, a;
  for (i = 0; i < sizeof key_names / sizeof key_names[0]; i++)
    if (!strcmp(word, key_names[i].name)) {
      *scan  = key_names[i].scan;
      *ascii = key_names[i].ascii;
      return 1;
    }
  if (sscanf(word, "%x:%x", &s, &a) == 2) {
    *scan  = (uint8_t)s;
    *ascii = (uint8_t)a;
    return 1;
  }
  if (strlen(word) == 1) return char_key(word[0], scan, ascii);
  return 0;
}

static int cmp_events(const void* a, const void* b) {
  const ScriptEvent *x = (const ScriptEvent*)a, *y = (const ScriptEvent*)b;
  if (x->ms != y->ms) return x->ms < y->ms ? -1 : 1;
  return 0;
}

static void script_load(const char* path) {
  FILE* f = fopen(path, "r");
  char  line[512];
  int   number = 0;
  if (!f) port_fatal("cannot open input script %s", path);
  while (fgets(line, sizeof line, f)) {
    char*         hash = strchr(line, '#');
    char          verb[32], arg[400], button[16];
    unsigned long ms;
    int           x, y, n;
    ScriptEvent   e;
    number++;
    if (hash) *hash = 0;
    memset(&e, 0, sizeof e);
    button[0] = 0;
    if (sscanf(line, "%lu %31s%n", &ms, verb, &n) < 2) continue;
    e.ms = (uint32_t)ms;
    if (!strcmp(verb, "key")) {
      if (sscanf(line + n, "%399s", arg) != 1 || !parse_key(arg, &e.scan, &e.ascii)) port_fatal("%s:%d: unknown key", path, number);
      e.kind = 0;
      script_add(e);
    } else if (!strcmp(verb, "text")) {
      char* p = line + n;
      while (*p == ' ' || *p == '\t') p++;
      for (; *p && *p != '\n' && *p != '\r'; p++) {
        if (!char_key(*p, &e.scan, &e.ascii)) port_fatal("%s:%d: no key for '%c'", path, number, *p);
        e.kind = 0;
        script_add(e);
      }
    } else if (!strcmp(verb, "move") || !strcmp(verb, "click") || !strcmp(verb, "press") || !strcmp(verb, "release")) {
      if (sscanf(line + n, "%d %d %15s", &x, &y, button) < 2) port_fatal("%s:%d: expected <x> <y>", path, number);
      e.x      = x;
      e.y      = y;
      e.button = !strcmp(button, "right") ? 1 : 0;
      if (!strcmp(verb, "move")) {
        e.kind = 1;
        script_add(e);
      } else if (!strcmp(verb, "press")) {
        e.kind = 2;
        script_add(e);
      } else if (!strcmp(verb, "release")) {
        e.kind = 3;
        script_add(e);
      } else {
        e.kind = 2;
        script_add(e);
        e.kind  = 3;
        e.ms   += 100;
        script_add(e);
      }
    } else {
      port_fatal("%s:%d: unknown event '%s'", path, number, verb);
    }
  }
  fclose(f);
  /* stable order by time (insertion order kept for equal times) */
  {
    unsigned i, j;
    for (i = 1; i < script_count; i++)
      for (j = i; j > 0 && cmp_events(&script[j - 1], &script[j]) > 0; j--) {
        ScriptEvent t = script[j - 1];
        script[j - 1] = script[j];
        script[j]     = t;
      }
  }
}

static void script_run(uint32_t now_ms) {
  while (script_next < script_count && script[script_next].ms <= now_ms) {
    const ScriptEvent* e = &script[script_next++];
    port_trace("input", "%u ms: script event %d", (unsigned)now_ms, e->kind);
    switch (e->kind) {
    case 0: port_input_key(e->scan, e->ascii); break;
    case 1: port_input_mouse_move(e->x, e->y); break;
    case 2: port_input_mouse_button(e->button, 1, e->x, e->y); break;
    case 3: port_input_mouse_button(e->button, 0, e->x, e->y); break;
    }
  }
}

/* ---- the pump ------------------------------------------------------------ */

static void headless_pump(void) {
  uint32_t now_ms;
  if (!script_loaded) {
    script_loaded = 1;
    if (port_options.input_path) script_load(port_options.input_path);
  }
  virtual_us += 1000;
  port_fm_clock();
  now_ms = (uint32_t)(virtual_us / 1000);
  script_run(now_ms);
  if (port_options.shot_dir && port_options.shot_every_ms && now_ms % port_options.shot_every_ms == 0) {
    char path[1024];
    present(1);
    snprintf(path, sizeof path, "%s/%08u.png", port_options.shot_dir, (unsigned)now_ms);
    if (plat_write_png(path) != 0) port_fatal("cannot write %s", path);
  }
  if (port_options.stop_ms && now_ms >= port_options.stop_ms) {
    present(1);
    if (port_options.png_path && plat_write_png(port_options.png_path) != 0) port_fatal("cannot write %s", port_options.png_path);
    port_trace("svc", "headless stop at %u ms", (unsigned)now_ms);
#if defined(__APPLE__) || defined(__linux__)
    if (getenv("OPEU_BACKTRACE")) {
      /* Diagnostic: where the game was waiting when the run stopped. */
      void* frames[64];
      int   n = backtrace(frames, 64);
      backtrace_symbols_fd(frames, n, 2);
    }
#endif
    port_exit(0);
  }
}

void port_pump(void) {
  static unsigned idle;
  static uint64_t last_poll_us;
  PlatEvent       ev;
  int             any = 0;
  uint64_t        now;

  if (port_options.headless) {
    headless_pump();
    return;
  }
  now = plat_ticks_us();
  if (now - last_poll_us < 500u) {
    if (++idle % 64u == 0) plat_sleep_ms(1);
    return;
  }
  last_poll_us = now;
  port_fm_clock();
  while (plat_poll_event(&ev)) {
    any = 1;
    switch (ev.type) {
    case PLAT_EV_QUIT:       port_exit(0); break;
    case PLAT_EV_KEY_DOWN:   port_input_key(ev.scancode, ev.ascii); break;
    case PLAT_EV_MOUSE_MOVE: port_input_mouse_move(ev.x, ev.y); break;
    case PLAT_EV_MOUSE_DOWN:
    case PLAT_EV_MOUSE_UP:   port_input_mouse_button(ev.button, ev.type == PLAT_EV_MOUSE_DOWN, ev.x, ev.y); break;
    default:                 break;
    }
  }
  /* --input in a window: the script's times are host milliseconds since
     * start (unattended windowed runs, e.g. to hear the music) */
  if (port_options.input_path) {
    if (!script_loaded) {
      script_loaded = 1;
      script_load(port_options.input_path);
    }
    script_run((uint32_t)(plat_ticks_us() / 1000));
  }
  now = plat_ticks_us();
  if (now - last_present_us >= 16667u) {
    last_present_us = now;
    present(0);
  }
  if (any)
    idle = 0;
  else if (++idle % 64u == 0)
    plat_sleep_ms(1);
}
