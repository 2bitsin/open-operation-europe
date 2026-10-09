/* BIOS keyboard (INT 16h) and mouse driver (INT 33h) natively.
 *
 * Keyboard: the BIOS type-ahead buffer holds 15 keys; each entry is the
 * word AH=scan code, AL=ASCII (US layout, from the platform). AH=01h
 * reports whether a key is waiting, AH=00h removes and returns one,
 * waiting (and pumping host events) while the buffer is empty. A key
 * pressed while the buffer is full is dropped (the BIOS beeps).
 *
 * Mouse: the functions MAIN issues, on a two-button driver in mode 12h
 * (virtual screen 640x480):
 *   AX=0 reset: AX=FFFFh (installed), BX=2 buttons; pointer hidden
 *        (counter -1), position centre (320,240), ranges the full screen.
 *   AX=1 show: counter+1 (at most 0); visible at 0.   AX=2 hide: counter-1.
 *   AX=3 state: BX buttons (bit0 left, bit1 right), CX=x, DX=y.
 *   AX=4 set position (clamped to the ranges).
 *   AX=7/8 x/y range min..max (swapped when min>max); position clamped.
 * The host pointer is absolute: each host motion moves the driver position
 * to the host position clamped to the ranges. A press and release between
 * two AX=3 queries is still reported once as pressed (the driver's state
 * changes are asynchronous on DOS; a host frame can contain both events).
 * The driver draws its pointer itself on the real hardware, never through
 * the game's VRAM; here the pointer is drawn into the presented frame
 * (port_input_draw_pointer) while it is shown.
 */
#include "hostinput.hpp"
#include "platform.hpp"
#include "port.hpp"
#include "services.hpp"

#include <string.h>

/* ---- keyboard ---------------------------------------------------------- */

#define KEY_BUFFER 16                     /* 15 usable entries, as the BIOS */
static uint16_t keys[KEY_BUFFER];
static unsigned key_head, key_tail;

void port_input_key(uint8_t scan, uint8_t ascii) {
  unsigned next = (key_tail + 1) % KEY_BUFFER;
  if (next == key_head) {
    port_trace("svc", "key %02x:%02x dropped (buffer full)", scan, ascii);
    return;
  }
  keys[key_tail] = (uint16_t)(scan << 8 | ascii);
  key_tail = next;
}

uint16_t port_key_available(void) {
  port_pump();
  return key_head != key_tail;
}

uint16_t port_key_read(void) {
  uint16_t k;
  while (key_head == key_tail) port_pump();
  k = keys[key_head];
  key_head = (key_head + 1) % KEY_BUFFER;
  port_trace("svc", "int16 read -> %04x", k);
  return k;
}

/* ---- mouse ------------------------------------------------------------- */

static struct {
  int x, y;                  /* driver position */
  int xmin, xmax, ymin, ymax;
  int counter;               /* 0 = visible */
  unsigned buttons;          /* current host state */
  unsigned latched;          /* pressed since the last AX=3 */
  int host_x, host_y;
  int drawn_x, drawn_y, drawn_visible;
} mouse = { 320, 240, 0, 639, 0, 479, -1, 0, 0, 320, 240, -1, -1, 0 };

static int clampi(int v, int lo, int hi) {
  return v < lo ? lo : v > hi ? hi : v;
}

static void mouse_clamp(void) {
  mouse.x = clampi(mouse.x, mouse.xmin, mouse.xmax);
  mouse.y = clampi(mouse.y, mouse.ymin, mouse.ymax);
}

void port_input_mouse_move(int x, int y) {
  mouse.host_x = x;
  mouse.host_y = y;
  mouse.x = x;
  mouse.y = y;
  mouse_clamp();
}

void port_input_mouse_button(int button, int down, int x, int y) {
  unsigned bit;
  if (button < 0 || button > 1) return;                 /* two-button driver */
  port_input_mouse_move(x, y);
  bit = 1u << button;
  if (down) {
    mouse.buttons |= bit;
    mouse.latched |= bit;
  } else {
    mouse.buttons &= ~bit;
  }
}

uint16_t port_mouse_reset(void) {
  mouse.counter = -1;
  mouse.xmin = 0;
  mouse.xmax = 639;
  mouse.ymin = 0;
  mouse.ymax = 479;
  mouse.x = 320;
  mouse.y = 240;
  mouse.latched = 0;
  port_trace("svc", "int33 reset");
  return 0xffff;
}

void port_mouse_show(void) {
  if (mouse.counter < 0) mouse.counter++;
}

void port_mouse_hide(void) {
  mouse.counter--;
}

void port_mouse_state(uint16_t* buttons, uint16_t* x, uint16_t* y) {
  port_pump();
  *buttons = (uint16_t)((mouse.buttons | mouse.latched) & 3);
  mouse.latched = 0;
  *x = (uint16_t)mouse.x;
  *y = (uint16_t)mouse.y;
}

void port_mouse_set_position(uint16_t x, uint16_t y) {
  mouse.x = (int16_t)x;
  mouse.y = (int16_t)y;
  mouse_clamp();
}

void port_mouse_x_range(uint16_t min, uint16_t max) {
  int a = (int16_t)min, b = (int16_t)max;
  mouse.xmin = a < b ? a : b;
  mouse.xmax = a < b ? b : a;
  mouse_clamp();
}

void port_mouse_y_range(uint16_t min, uint16_t max) {
  int a = (int16_t)min, b = (int16_t)max;
  mouse.ymin = a < b ? a : b;
  mouse.ymax = a < b ? b : a;
  mouse_clamp();
}

/* ---- pointer overlay ----------------------------------------------------- */

/* The default arrow of the Microsoft driver: '#' outline (black), '.' body
 * (white), hot spot at the tip. */
static const char* const arrow[16] = {
  "#",         "##",         "#.#",         "#..#",    "#...#",    "#....#",   "#.....#",   "#......#",
  "#.......#", "#........#", "#.....#####", "#..#..#", "#.# #..#", "##  #..#", "     #..#", "     ####",
};

#define POINTER_BLACK 254
#define POINTER_WHITE 255

int port_input_pointer_dirty(void) {
  int visible = mouse.counter >= 0;
  return visible != mouse.drawn_visible || (visible && (mouse.x != mouse.drawn_x || mouse.y != mouse.drawn_y));
}

int port_input_draw_pointer(uint8_t* frame, uint8_t pal_rgb8[768]) {
  int r, c;
  mouse.drawn_visible = mouse.counter >= 0;
  mouse.drawn_x = mouse.x;
  mouse.drawn_y = mouse.y;
  if (!mouse.drawn_visible) return 0;
  pal_rgb8[POINTER_BLACK * 3 + 0] = 0;
  pal_rgb8[POINTER_BLACK * 3 + 1] = 0;
  pal_rgb8[POINTER_BLACK * 3 + 2] = 0;
  pal_rgb8[POINTER_WHITE * 3 + 0] = 255;
  pal_rgb8[POINTER_WHITE * 3 + 1] = 255;
  pal_rgb8[POINTER_WHITE * 3 + 2] = 255;
  for (r = 0; r < 16; r++) {
    int y = mouse.y + r;
    if (y < 0 || y >= 480) continue;
    for (c = 0; arrow[r][c]; c++) {
      int x = mouse.x + c;
      if (x < 0 || x >= 640 || arrow[r][c] == ' ') continue;
      frame[y * 640 + x] = arrow[r][c] == '#' ? POINTER_BLACK : POINTER_WHITE;
    }
  }
  return 1;
}
