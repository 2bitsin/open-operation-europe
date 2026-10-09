/* SDL3 implementation of port/platform/platform.h.
 *
 * Portable C99 + SDL3 only: builds for macOS, Linux, Windows and
 * Emscripten.  Video is an indexed framebuffer converted through the
 * palette into a streaming XRGB8888 texture, shown with SDL's logical
 * presentation (integer scaling + letterbox).  Input is translated to PC
 * set-1 make codes plus the BIOS INT 16h ASCII of a US keyboard.  Files are
 * stdio FILE* handles on paths resolved case-insensitively inside the data
 * directory.  Audio is a pull callback on an SDL_AudioStream.
 *
 * Emscripten (web build, port/README.md "Web build"): the game's waits are
 * polling loops that never return to a main loop, so the build uses
 * ASYNCIFY (or JSPI) and this backend hands control back to the browser at
 * exactly two places, both marked __EMSCRIPTEN__: after each presented
 * frame (so the canvas is shown and input events are delivered) and in
 * plat_sleep_ms() (the pump's idle sleep).  SDL's own automatic
 * emscripten_sleep() calls are switched off so that these are the only
 * yields.  The page's CSS sizes the canvas to an integer multiple of the
 * frame (shell.html) and SDL follows that size on window resize events, so
 * the renderer's integer scaling applies there too; the canvas takes the
 * keyboard events itself and has no vsync setting (the browser paces
 * frames).
 */
#include "platform.h"

#include <SDL3/SDL.h>
/* The headless --png writer: stb_image_write (conan stb), implemented here,
 * its only user. */
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 1: hide the host cursor while it is over the window (the game draws its
 * own mouse pointer).  0: leave the OS cursor visible. */
#ifndef PLAT_HIDE_CURSOR
#define PLAT_HIDE_CURSOR 1
#endif

/* SDL_LOGICAL_PRESENTATION_INTEGER_SCALE or SDL_LOGICAL_PRESENTATION_LETTERBOX */
#ifndef PLAT_PRESENTATION
/* The web shell sizes the canvas to an integer multiple of 640x480 (see
 * port/platform/web/shell.html), so integer scaling applies there too. */
#define PLAT_PRESENTATION SDL_LOGICAL_PRESENTATION_INTEGER_SCALE
#endif

#define PLAT_PATH_MAX 1024

/* ---- state -------------------------------------------------------------- */

static struct {
    int inited;
    int headless;
    int sdl_flags;
    uint64_t t0_ns;

    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    int tex_w, tex_h;

    uint8_t *frame;          /* copy of the last indexed frame */
    int frame_w, frame_h;
    uint8_t palette[256 * 3];
    int have_frame;

    SDL_AudioStream *audio;
    PlatAudioFn audio_fn;
    void *audio_user;
    int audio_rate;

    char data_dir[PLAT_PATH_MAX];
} P;

/* ---- lifecycle ---------------------------------------------------------- */

int plat_init(const PlatConfig *cfg)
{
    int w, h, scale;
    SDL_WindowFlags wflags;

    if (P.inited)
        return 0;
    if (!cfg)
        return -1;
    P.headless = cfg->headless ? 1 : 0;
    w = cfg->width > 0 ? cfg->width : 640;
    h = cfg->height > 0 ? cfg->height : 480;
    scale = cfg->scale >= 1 ? cfg->scale : 1;

    if (P.data_dir[0] == '\0')
        plat_set_data_dir("binaries");

    if (P.headless) {
        /* No subsystems: timers and the filesystem helpers need none. */
        if (!SDL_Init(0)) {
            fprintf(stderr, "plat_init: SDL_Init: %s\n", SDL_GetError());
            return -1;
        }
        P.sdl_flags = 0;
    } else {
        SDL_SetAppMetadata(cfg->title ? cfg->title : "Operation Europe",
                           NULL, NULL);
#ifdef __EMSCRIPTEN__
        /* Keys go to the canvas (focused by the page's click-to-start), and
         * only this backend yields to the browser (see the file comment). */
        SDL_SetHint(SDL_HINT_EMSCRIPTEN_KEYBOARD_ELEMENT, "#canvas");
        SDL_SetHint(SDL_HINT_EMSCRIPTEN_ASYNCIFY, "0");
#endif
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
            fprintf(stderr, "plat_init: SDL_Init(video): %s\n", SDL_GetError());
            return -1;
        }
        P.sdl_flags = SDL_INIT_VIDEO | SDL_INIT_EVENTS;
        /* Audio is optional: continue silently if there is no device. */
        if (SDL_InitSubSystem(SDL_INIT_AUDIO))
            P.sdl_flags |= SDL_INIT_AUDIO;
        else
            fprintf(stderr, "plat_init: no audio: %s\n", SDL_GetError());

        wflags = SDL_WINDOW_RESIZABLE;
        if (cfg->fullscreen)
            wflags |= SDL_WINDOW_FULLSCREEN;
        P.window = SDL_CreateWindow(cfg->title ? cfg->title : "Operation Europe",
                                    w * scale, h * scale, wflags);
        if (!P.window) {
            fprintf(stderr, "plat_init: SDL_CreateWindow: %s\n", SDL_GetError());
            SDL_Quit();
            return -1;
        }
        P.renderer = SDL_CreateRenderer(P.window, NULL);
        if (!P.renderer) {
            fprintf(stderr, "plat_init: SDL_CreateRenderer: %s\n", SDL_GetError());
            SDL_DestroyWindow(P.window);
            P.window = NULL;
            SDL_Quit();
            return -1;
        }
#ifndef __EMSCRIPTEN__
        SDL_SetRenderVSync(P.renderer, 1);
#endif
        SDL_SetRenderLogicalPresentation(P.renderer, w, h, PLAT_PRESENTATION);
#if PLAT_HIDE_CURSOR
        SDL_HideCursor();
#endif
        /* Never deliver text-input events; we derive ASCII ourselves. */
        SDL_StopTextInput(P.window);
        SDL_SetRenderDrawColor(P.renderer, 0, 0, 0, 255);
        SDL_RenderClear(P.renderer);
        SDL_RenderPresent(P.renderer);
    }
    P.t0_ns = SDL_GetTicksNS();
    P.inited = 1;
    return 0;
}

void plat_shutdown(void)
{
    if (!P.inited)
        return;
    if (P.audio) {
        SDL_DestroyAudioStream(P.audio);
        P.audio = NULL;
    }
    if (P.texture) {
        SDL_DestroyTexture(P.texture);
        P.texture = NULL;
    }
    if (P.renderer) {
        SDL_DestroyRenderer(P.renderer);
        P.renderer = NULL;
    }
    if (P.window) {
        SDL_DestroyWindow(P.window);
        P.window = NULL;
    }
    free(P.frame);
    P.frame = NULL;
    P.have_frame = 0;
    SDL_Quit();
    P.inited = 0;
}

/* ---- video -------------------------------------------------------------- */

void plat_present(const uint8_t *pixels, int width, int height,
                  const uint8_t palette[256 * 3])
{
    size_t n;
    uint32_t lut[256];
    int i, y;
    void *tp;
    int pitch;

    if (!pixels || !palette || width <= 0 || height <= 0)
        return;
    n = (size_t)width * (size_t)height;
    if (!P.frame || P.frame_w != width || P.frame_h != height) {
        uint8_t *nf = (uint8_t *)realloc(P.frame, n);
        if (!nf)
            return;
        P.frame = nf;
        P.frame_w = width;
        P.frame_h = height;
    }
    memcpy(P.frame, pixels, n);
    memcpy(P.palette, palette, sizeof P.palette);
    P.have_frame = 1;

    if (!P.renderer)
        return;

    if (!P.texture || P.tex_w != width || P.tex_h != height) {
        if (P.texture)
            SDL_DestroyTexture(P.texture);
        P.texture = SDL_CreateTexture(P.renderer, SDL_PIXELFORMAT_XRGB8888,
                                      SDL_TEXTUREACCESS_STREAMING,
                                      width, height);
        if (!P.texture)
            return;
        SDL_SetTextureScaleMode(P.texture, SDL_SCALEMODE_NEAREST);
        P.tex_w = width;
        P.tex_h = height;
        SDL_SetRenderLogicalPresentation(P.renderer, width, height,
                                         PLAT_PRESENTATION);
    }

    for (i = 0; i < 256; i++)
        lut[i] = 0xFF000000u | ((uint32_t)palette[i * 3] << 16) |
                 ((uint32_t)palette[i * 3 + 1] << 8) | (uint32_t)palette[i * 3 + 2];

    if (SDL_LockTexture(P.texture, NULL, &tp, &pitch)) {
        for (y = 0; y < height; y++) {
            uint32_t *dst = (uint32_t *)((uint8_t *)tp + (size_t)y * (size_t)pitch);
            const uint8_t *src = pixels + (size_t)y * (size_t)width;
            int x;
            for (x = 0; x < width; x++)
                dst[x] = lut[src[x]];
        }
        SDL_UnlockTexture(P.texture);
    }
    SDL_SetRenderDrawColor(P.renderer, 0, 0, 0, 255);
    SDL_RenderClear(P.renderer);
    SDL_RenderTexture(P.renderer, P.texture, NULL, NULL);
    SDL_RenderPresent(P.renderer);
#ifdef __EMSCRIPTEN__
    /* Return to the browser so it shows the frame and queues input. */
    emscripten_sleep(0);
#endif
}

int plat_write_png(const char *path)
{
    uint8_t *rgb;
    size_t n, i;
    int rc;

    if (!P.have_frame || !path)
        return -1;
    n = (size_t)P.frame_w * (size_t)P.frame_h;
    rgb = (uint8_t *)malloc(n * 3u);
    if (!rgb)
        return -1;
    for (i = 0; i < n; i++) {
        const uint8_t *c = P.palette + (size_t)P.frame[i] * 3u;
        rgb[i * 3] = c[0];
        rgb[i * 3 + 1] = c[1];
        rgb[i * 3 + 2] = c[2];
    }
    /* 8-bit truecolour, filter type 0 on every row (as the port's own
     * writer had it, so the image readers of the integration suites need
     * no unfiltering); stb compresses the IDAT where that writer stored it. */
    stbi_write_force_png_filter = 0;
    rc = stbi_write_png(path, P.frame_w, P.frame_h, 3, rgb, P.frame_w * 3) ? 0 : -1;
    free(rgb);
    return rc;
}

/* ---- input -------------------------------------------------------------- */

/* Key kinds for the ASCII computation. */
enum { K_PLAIN = 0, K_LETTER, K_KEYPAD, K_NOASCII };

typedef struct KeyMap {
    SDL_Scancode sc;
    uint8_t code;      /* set-1 make code */
    uint8_t ext;       /* 0xE0 prefix */
    uint8_t kind;
    uint8_t lo, hi;    /* US ASCII unshifted / shifted */
    uint8_t ctrl;      /* ASCII with Ctrl held (0 = none) */
} KeyMap;

static const KeyMap keymap[] = {
    /* letters */
    { SDL_SCANCODE_A, 0x1E, 0, K_LETTER, 'a', 'A', 0x01 },
    { SDL_SCANCODE_B, 0x30, 0, K_LETTER, 'b', 'B', 0x02 },
    { SDL_SCANCODE_C, 0x2E, 0, K_LETTER, 'c', 'C', 0x03 },
    { SDL_SCANCODE_D, 0x20, 0, K_LETTER, 'd', 'D', 0x04 },
    { SDL_SCANCODE_E, 0x12, 0, K_LETTER, 'e', 'E', 0x05 },
    { SDL_SCANCODE_F, 0x21, 0, K_LETTER, 'f', 'F', 0x06 },
    { SDL_SCANCODE_G, 0x22, 0, K_LETTER, 'g', 'G', 0x07 },
    { SDL_SCANCODE_H, 0x23, 0, K_LETTER, 'h', 'H', 0x08 },
    { SDL_SCANCODE_I, 0x17, 0, K_LETTER, 'i', 'I', 0x09 },
    { SDL_SCANCODE_J, 0x24, 0, K_LETTER, 'j', 'J', 0x0A },
    { SDL_SCANCODE_K, 0x25, 0, K_LETTER, 'k', 'K', 0x0B },
    { SDL_SCANCODE_L, 0x26, 0, K_LETTER, 'l', 'L', 0x0C },
    { SDL_SCANCODE_M, 0x32, 0, K_LETTER, 'm', 'M', 0x0D },
    { SDL_SCANCODE_N, 0x31, 0, K_LETTER, 'n', 'N', 0x0E },
    { SDL_SCANCODE_O, 0x18, 0, K_LETTER, 'o', 'O', 0x0F },
    { SDL_SCANCODE_P, 0x19, 0, K_LETTER, 'p', 'P', 0x10 },
    { SDL_SCANCODE_Q, 0x10, 0, K_LETTER, 'q', 'Q', 0x11 },
    { SDL_SCANCODE_R, 0x13, 0, K_LETTER, 'r', 'R', 0x12 },
    { SDL_SCANCODE_S, 0x1F, 0, K_LETTER, 's', 'S', 0x13 },
    { SDL_SCANCODE_T, 0x14, 0, K_LETTER, 't', 'T', 0x14 },
    { SDL_SCANCODE_U, 0x16, 0, K_LETTER, 'u', 'U', 0x15 },
    { SDL_SCANCODE_V, 0x2F, 0, K_LETTER, 'v', 'V', 0x16 },
    { SDL_SCANCODE_W, 0x11, 0, K_LETTER, 'w', 'W', 0x17 },
    { SDL_SCANCODE_X, 0x2D, 0, K_LETTER, 'x', 'X', 0x18 },
    { SDL_SCANCODE_Y, 0x15, 0, K_LETTER, 'y', 'Y', 0x19 },
    { SDL_SCANCODE_Z, 0x2C, 0, K_LETTER, 'z', 'Z', 0x1A },
    /* digit row */
    { SDL_SCANCODE_1, 0x02, 0, K_PLAIN, '1', '!', 0 },
    { SDL_SCANCODE_2, 0x03, 0, K_PLAIN, '2', '@', 0 },
    { SDL_SCANCODE_3, 0x04, 0, K_PLAIN, '3', '#', 0 },
    { SDL_SCANCODE_4, 0x05, 0, K_PLAIN, '4', '$', 0 },
    { SDL_SCANCODE_5, 0x06, 0, K_PLAIN, '5', '%', 0 },
    { SDL_SCANCODE_6, 0x07, 0, K_PLAIN, '6', '^', 0x1E },
    { SDL_SCANCODE_7, 0x08, 0, K_PLAIN, '7', '&', 0 },
    { SDL_SCANCODE_8, 0x09, 0, K_PLAIN, '8', '*', 0 },
    { SDL_SCANCODE_9, 0x0A, 0, K_PLAIN, '9', '(', 0 },
    { SDL_SCANCODE_0, 0x0B, 0, K_PLAIN, '0', ')', 0 },
    { SDL_SCANCODE_MINUS, 0x0C, 0, K_PLAIN, '-', '_', 0x1F },
    { SDL_SCANCODE_EQUALS, 0x0D, 0, K_PLAIN, '=', '+', 0 },
    { SDL_SCANCODE_BACKSPACE, 0x0E, 0, K_PLAIN, 0x08, 0x08, 0x7F },
    { SDL_SCANCODE_TAB, 0x0F, 0, K_PLAIN, 0x09, 0x00, 0 },
    { SDL_SCANCODE_LEFTBRACKET, 0x1A, 0, K_PLAIN, '[', '{', 0x1B },
    { SDL_SCANCODE_RIGHTBRACKET, 0x1B, 0, K_PLAIN, ']', '}', 0x1D },
    { SDL_SCANCODE_RETURN, 0x1C, 0, K_PLAIN, 0x0D, 0x0D, 0x0A },
    { SDL_SCANCODE_SEMICOLON, 0x27, 0, K_PLAIN, ';', ':', 0 },
    { SDL_SCANCODE_APOSTROPHE, 0x28, 0, K_PLAIN, '\'', '"', 0 },
    { SDL_SCANCODE_GRAVE, 0x29, 0, K_PLAIN, '`', '~', 0 },
    { SDL_SCANCODE_BACKSLASH, 0x2B, 0, K_PLAIN, '\\', '|', 0x1C },
    { SDL_SCANCODE_NONUSBACKSLASH, 0x56, 0, K_PLAIN, '\\', '|', 0x1C },
    { SDL_SCANCODE_COMMA, 0x33, 0, K_PLAIN, ',', '<', 0 },
    { SDL_SCANCODE_PERIOD, 0x34, 0, K_PLAIN, '.', '>', 0 },
    { SDL_SCANCODE_SLASH, 0x35, 0, K_PLAIN, '/', '?', 0 },
    { SDL_SCANCODE_SPACE, 0x39, 0, K_PLAIN, ' ', ' ', ' ' },
    { SDL_SCANCODE_ESCAPE, 0x01, 0, K_PLAIN, 0x1B, 0x1B, 0x1B },
    /* modifiers and locks */
    { SDL_SCANCODE_LCTRL, 0x1D, 0, K_NOASCII, 0, 0, 0 },
    { SDL_SCANCODE_RCTRL, 0x1D, 1, K_NOASCII, 0, 0, 0 },
    { SDL_SCANCODE_LSHIFT, 0x2A, 0, K_NOASCII, 0, 0, 0 },
    { SDL_SCANCODE_RSHIFT, 0x36, 0, K_NOASCII, 0, 0, 0 },
    { SDL_SCANCODE_LALT, 0x38, 0, K_NOASCII, 0, 0, 0 },
    { SDL_SCANCODE_RALT, 0x38, 1, K_NOASCII, 0, 0, 0 },
    { SDL_SCANCODE_LGUI, 0x5B, 1, K_NOASCII, 0, 0, 0 },
    { SDL_SCANCODE_RGUI, 0x5C, 1, K_NOASCII, 0, 0, 0 },
    { SDL_SCANCODE_APPLICATION, 0x5D, 1, K_NOASCII, 0, 0, 0 },
    { SDL_SCANCODE_CAPSLOCK, 0x3A, 0, K_NOASCII, 0, 0, 0 },
    { SDL_SCANCODE_NUMLOCKCLEAR, 0x45, 0, K_NOASCII, 0, 0, 0 },
    { SDL_SCANCODE_SCROLLLOCK, 0x46, 0, K_NOASCII, 0, 0, 0 },
    { SDL_SCANCODE_PRINTSCREEN, 0x37, 1, K_NOASCII, 0, 0, 0 },
    /* function keys */
    { SDL_SCANCODE_F1, 0x3B, 0, K_NOASCII, 0, 0, 0 },
    { SDL_SCANCODE_F2, 0x3C, 0, K_NOASCII, 0, 0, 0 },
    { SDL_SCANCODE_F3, 0x3D, 0, K_NOASCII, 0, 0, 0 },
    { SDL_SCANCODE_F4, 0x3E, 0, K_NOASCII, 0, 0, 0 },
    { SDL_SCANCODE_F5, 0x3F, 0, K_NOASCII, 0, 0, 0 },
    { SDL_SCANCODE_F6, 0x40, 0, K_NOASCII, 0, 0, 0 },
    { SDL_SCANCODE_F7, 0x41, 0, K_NOASCII, 0, 0, 0 },
    { SDL_SCANCODE_F8, 0x42, 0, K_NOASCII, 0, 0, 0 },
    { SDL_SCANCODE_F9, 0x43, 0, K_NOASCII, 0, 0, 0 },
    { SDL_SCANCODE_F10, 0x44, 0, K_NOASCII, 0, 0, 0 },
    { SDL_SCANCODE_F11, 0x57, 0, K_NOASCII, 0, 0, 0 },
    { SDL_SCANCODE_F12, 0x58, 0, K_NOASCII, 0, 0, 0 },
    /* grey navigation block (0xE0 prefixed) */
    { SDL_SCANCODE_INSERT, 0x52, 1, K_NOASCII, 0, 0, 0 },
    { SDL_SCANCODE_DELETE, 0x53, 1, K_NOASCII, 0, 0, 0 },
    { SDL_SCANCODE_HOME, 0x47, 1, K_NOASCII, 0, 0, 0 },
    { SDL_SCANCODE_END, 0x4F, 1, K_NOASCII, 0, 0, 0 },
    { SDL_SCANCODE_PAGEUP, 0x49, 1, K_NOASCII, 0, 0, 0 },
    { SDL_SCANCODE_PAGEDOWN, 0x51, 1, K_NOASCII, 0, 0, 0 },
    { SDL_SCANCODE_UP, 0x48, 1, K_NOASCII, 0, 0, 0 },
    { SDL_SCANCODE_DOWN, 0x50, 1, K_NOASCII, 0, 0, 0 },
    { SDL_SCANCODE_LEFT, 0x4B, 1, K_NOASCII, 0, 0, 0 },
    { SDL_SCANCODE_RIGHT, 0x4D, 1, K_NOASCII, 0, 0, 0 },
    /* keypad */
    { SDL_SCANCODE_KP_DIVIDE, 0x35, 1, K_PLAIN, '/', '/', 0 },
    { SDL_SCANCODE_KP_MULTIPLY, 0x37, 0, K_PLAIN, '*', '*', 0 },
    { SDL_SCANCODE_KP_MINUS, 0x4A, 0, K_PLAIN, '-', '-', 0 },
    { SDL_SCANCODE_KP_PLUS, 0x4E, 0, K_PLAIN, '+', '+', 0 },
    { SDL_SCANCODE_KP_ENTER, 0x1C, 1, K_PLAIN, 0x0D, 0x0D, 0x0A },
    { SDL_SCANCODE_KP_EQUALS, 0x0D, 0, K_PLAIN, '=', '=', 0 },
    { SDL_SCANCODE_KP_7, 0x47, 0, K_KEYPAD, '7', 0, 0 },
    { SDL_SCANCODE_KP_8, 0x48, 0, K_KEYPAD, '8', 0, 0 },
    { SDL_SCANCODE_KP_9, 0x49, 0, K_KEYPAD, '9', 0, 0 },
    { SDL_SCANCODE_KP_4, 0x4B, 0, K_KEYPAD, '4', 0, 0 },
    { SDL_SCANCODE_KP_5, 0x4C, 0, K_KEYPAD, '5', 0, 0 },
    { SDL_SCANCODE_KP_6, 0x4D, 0, K_KEYPAD, '6', 0, 0 },
    { SDL_SCANCODE_KP_1, 0x4F, 0, K_KEYPAD, '1', 0, 0 },
    { SDL_SCANCODE_KP_2, 0x50, 0, K_KEYPAD, '2', 0, 0 },
    { SDL_SCANCODE_KP_3, 0x51, 0, K_KEYPAD, '3', 0, 0 },
    { SDL_SCANCODE_KP_0, 0x52, 0, K_KEYPAD, '0', 0, 0 },
    { SDL_SCANCODE_KP_PERIOD, 0x53, 0, K_KEYPAD, '.', 0, 0 },
};

static const KeyMap *keymap_find(SDL_Scancode sc)
{
    static const KeyMap *lut[SDL_SCANCODE_COUNT];
    static int built;
    if (!built) {
        size_t i;
        for (i = 0; i < sizeof keymap / sizeof keymap[0]; i++)
            if ((int)keymap[i].sc < (int)SDL_SCANCODE_COUNT)
                lut[keymap[i].sc] = &keymap[i];
        built = 1;
    }
    if ((int)sc < 0 || (int)sc >= (int)SDL_SCANCODE_COUNT)
        return NULL;
    return lut[sc];
}

/* BIOS INT 16h ASCII of a key for the given SDL modifier state. */
static uint8_t key_ascii(const KeyMap *k, SDL_Keymod mod)
{
    int shift = (mod & SDL_KMOD_SHIFT) != 0;
    if (k->kind == K_NOASCII)
        return 0;
    if (mod & SDL_KMOD_ALT)
        return 0;                       /* Alt combinations: ASCII 00 */
    if (mod & SDL_KMOD_CTRL)
        return k->ctrl;
    switch (k->kind) {
    case K_LETTER:
        if (mod & SDL_KMOD_CAPS)
            shift = !shift;
        return shift ? k->hi : k->lo;
    case K_KEYPAD:
        /* Digits only when Num Lock xor Shift; otherwise cursor keys. */
        return (((mod & SDL_KMOD_NUM) != 0) != shift) ? k->lo : 0;
    default:
        return shift ? k->hi : k->lo;
    }
}

static int clamp_i(int v, int lo, int hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

static void mouse_pos(SDL_Event *e, float fx, float fy, int *x, int *y)
{
    int w = P.tex_w > 0 ? P.tex_w : (P.frame_w > 0 ? P.frame_w : 640);
    int h = P.tex_h > 0 ? P.tex_h : (P.frame_h > 0 ? P.frame_h : 480);
    (void)e;
    *x = clamp_i((int)SDL_floorf(fx), 0, w - 1);
    *y = clamp_i((int)SDL_floorf(fy), 0, h - 1);
}

int plat_poll_event(PlatEvent *ev)
{
    SDL_Event e;

    if (!ev)
        return 0;
    if (!P.inited || P.headless)
        return 0;
    while (SDL_PollEvent(&e)) {
        memset(ev, 0, sizeof *ev);
        switch (e.type) {
        case SDL_EVENT_QUIT:
            ev->type = PLAT_EV_QUIT;
            return 1;
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP: {
            /* SDL key repeat arrives as further KEY_DOWN events (repeat=1),
             * which is exactly the DOS typematic behaviour. */
            const KeyMap *k = keymap_find(e.key.scancode);
            if (!k)
                break;
            ev->type = e.type == SDL_EVENT_KEY_DOWN ? PLAT_EV_KEY_DOWN
                                                    : PLAT_EV_KEY_UP;
            ev->scancode = k->code;
            ev->extended = k->ext;
            ev->ascii = ev->type == PLAT_EV_KEY_DOWN ? key_ascii(k, e.key.mod) : 0;
            return 1;
        }
        case SDL_EVENT_MOUSE_MOTION:
            SDL_ConvertEventToRenderCoordinates(P.renderer, &e);
            ev->type = PLAT_EV_MOUSE_MOVE;
            mouse_pos(&e, e.motion.x, e.motion.y, &ev->x, &ev->y);
            return 1;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP: {
            int b;
            if (e.button.button == SDL_BUTTON_LEFT) b = 0;
            else if (e.button.button == SDL_BUTTON_RIGHT) b = 1;
            else if (e.button.button == SDL_BUTTON_MIDDLE) b = 2;
            else break;
            SDL_ConvertEventToRenderCoordinates(P.renderer, &e);
            ev->type = e.type == SDL_EVENT_MOUSE_BUTTON_DOWN ? PLAT_EV_MOUSE_DOWN
                                                             : PLAT_EV_MOUSE_UP;
            ev->button = (uint8_t)b;
            mouse_pos(&e, e.button.x, e.button.y, &ev->x, &ev->y);
            return 1;
        }
        default:
            break;
        }
    }
    memset(ev, 0, sizeof *ev);
    return 0;
}

uint8_t plat_shift_flags(void)
{
    SDL_Keymod m;
    uint8_t f = 0;
    if (!P.inited || P.headless)
        return 0;
    m = SDL_GetModState();
    if (m & SDL_KMOD_RSHIFT) f |= 0x01;
    if (m & SDL_KMOD_LSHIFT) f |= 0x02;
    if (m & SDL_KMOD_CTRL)   f |= 0x04;
    if (m & SDL_KMOD_ALT)    f |= 0x08;
    if (m & SDL_KMOD_SCROLL) f |= 0x10;
    if (m & SDL_KMOD_NUM)    f |= 0x20;
    if (m & SDL_KMOD_CAPS)   f |= 0x40;
    return f;
}

/* ---- time --------------------------------------------------------------- */

uint64_t plat_ticks_us(void)
{
    return (SDL_GetTicksNS() - P.t0_ns) / 1000u;
}

void plat_sleep_ms(int ms)
{
#ifdef __EMSCRIPTEN__
    /* Yield to the browser event loop (ASYNCIFY/JSPI unwinds the stack and
     * resumes here from a timer). */
    if (ms >= 0 && !P.headless)
        emscripten_sleep((unsigned)ms);
#else
    if (ms > 0)
        SDL_Delay((Uint32)ms);
#endif
}

/* ---- files -------------------------------------------------------------- */

void plat_set_data_dir(const char *dir)
{
    size_t n;
    if (!dir || !*dir)
        dir = ".";
    SDL_strlcpy(P.data_dir, dir, sizeof P.data_dir);
    n = strlen(P.data_dir);
    while (n > 1 && (P.data_dir[n - 1] == '/' || P.data_dir[n - 1] == '\\'))
        P.data_dir[--n] = '\0';
}

const char *plat_data_dir(void)
{
    if (P.data_dir[0] == '\0')
        plat_set_data_dir("binaries");
    return P.data_dir;
}

typedef struct FindCtx {
    const char *want;
    char found[256];
    int ok;
} FindCtx;

static SDL_EnumerationResult SDLCALL find_cb(void *ud, const char *dirname,
                                             const char *fname)
{
    FindCtx *c = (FindCtx *)ud;
    (void)dirname;
    if (SDL_strcasecmp(fname, c->want) == 0) {
        SDL_strlcpy(c->found, fname, sizeof c->found);
        c->ok = 1;
        if (SDL_strcmp(fname, c->want) == 0)
            return SDL_ENUM_SUCCESS;     /* exact match wins */
    }
    return SDL_ENUM_CONTINUE;
}

static int path_exists(const char *path)
{
    return SDL_GetPathInfo(path, NULL);
}

/* Resolve a DOS name (may contain '\' or '/' separators) inside the data
 * directory, matching every component case-insensitively.  When the final
 * component is missing and `create` is set, it is appended in upper case
 * (the DOS spelling).  Returns 1 if the file exists, 0 if a creatable path
 * was produced, -1 on failure. */
static int resolve(const char *dos_name, char *out, size_t outsz, int create)
{
    char comp[256];
    const char *p = dos_name;
    size_t len;

    if (!dos_name || !*dos_name)
        return -1;
    SDL_strlcpy(out, plat_data_dir(), outsz);

    while (*p) {
        size_t n = 0;
        int last;
        while (*p == '\\' || *p == '/')
            p++;
        if (!*p)
            break;
        while (p[n] && p[n] != '\\' && p[n] != '/')
            n++;
        if (n >= sizeof comp)
            return -1;
        memcpy(comp, p, n);
        comp[n] = '\0';
        p += n;
        while (*p == '\\' || *p == '/')
            p++;
        last = (*p == '\0');

        len = strlen(out);
        if (len + 1 + n + 1 > outsz)
            return -1;
        /* Fast path: the exact spelling exists. */
        out[len] = '/';
        memcpy(out + len + 1, comp, n + 1);
        if (path_exists(out))
            continue;
        out[len] = '\0';
        {
            FindCtx c;
            c.want = comp;
            c.found[0] = '\0';
            c.ok = 0;
            SDL_EnumerateDirectory(out, find_cb, &c);
            if (c.ok) {
                size_t fl = strlen(c.found);
                if (len + 1 + fl + 1 > outsz)
                    return -1;
                out[len] = '/';
                memcpy(out + len + 1, c.found, fl + 1);
                continue;
            }
        }
        if (!last || !create)
            return -1;
        {
            size_t i;
            out[len] = '/';
            for (i = 0; i <= n; i++)
                out[len + 1 + i] = (char)SDL_toupper((unsigned char)comp[i]);
        }
        return 0;
    }
    return 1;
}

void *plat_fopen(const char *dos_name, const char *mode)
{
    char path[PLAT_PATH_MAX];
    int creating;
    int r;

    if (!dos_name || !mode)
        return NULL;
    creating = strchr(mode, 'w') != NULL || strchr(mode, 'a') != NULL;
    r = resolve(dos_name, path, sizeof path, creating);
    if (r < 0 || (r == 0 && !creating))
        return NULL;
    return fopen(path, mode);
}

int plat_fclose(void *f)
{
    return f ? fclose((FILE *)f) : -1;
}

size_t plat_fread(void *buf, size_t n, void *f)
{
    if (!f || !buf || n == 0)
        return 0;
    return fread(buf, 1, n, (FILE *)f);
}

size_t plat_fwrite(const void *buf, size_t n, void *f)
{
    if (!f || !buf || n == 0)
        return 0;
    return fwrite(buf, 1, n, (FILE *)f);
}

long plat_fseek(void *f, long off, int whence)
{
    if (!f)
        return -1;
    if (fseek((FILE *)f, off, whence) != 0)
        return -1;
    return ftell((FILE *)f);
}

long plat_fsize(void *f)
{
    long cur, end;
    if (!f)
        return -1;
    cur = ftell((FILE *)f);
    if (cur < 0 || fseek((FILE *)f, 0, SEEK_END) != 0)
        return -1;
    end = ftell((FILE *)f);
    fseek((FILE *)f, cur, SEEK_SET);
    return end;
}

int plat_fexists(const char *dos_name)
{
    char path[PLAT_PATH_MAX];
    return resolve(dos_name, path, sizeof path, 0) == 1;
}

int plat_fdelete(const char *dos_name)
{
    char path[PLAT_PATH_MAX];
    if (resolve(dos_name, path, sizeof path, 0) != 1)
        return -1;
    return remove(path) == 0 ? 0 : -1;
}

/* DOS semantics: fails if the source is missing or the target exists. */
int plat_frename(const char *from, const char *to)
{
    char src[PLAT_PATH_MAX], dst[PLAT_PATH_MAX];
    int r;
    if (resolve(from, src, sizeof src, 0) != 1)
        return -1;
    r = resolve(to, dst, sizeof dst, 1);
    if (r != 0)
        return -1;
    return rename(src, dst) == 0 ? 0 : -1;
}

/* ---- audio -------------------------------------------------------------- */

static void SDLCALL audio_cb(void *ud, SDL_AudioStream *stream,
                             int additional, int total)
{
    int16_t buf[1024 * 2];
    (void)ud;
    (void)total;
    /* additional is in bytes; one frame is 4 bytes (S16 stereo). */
    while (additional > 0) {
        int frames = additional / 4;
        if (frames <= 0)
            break;
        if (frames > 1024)
            frames = 1024;
        if (P.audio_fn)
            P.audio_fn(P.audio_user, buf, frames, P.audio_rate);
        else
            memset(buf, 0, (size_t)frames * 4u);
        SDL_PutAudioStreamData(stream, buf, frames * 4);
        additional -= frames * 4;
    }
}

int plat_audio_open(int rate, PlatAudioFn fn, void *user)
{
    SDL_AudioSpec spec;

    if (!P.inited || P.headless)
        return 0;
    if (!(P.sdl_flags & SDL_INIT_AUDIO))
        return -1;
    if (rate <= 0)
        rate = 44100;
    if (P.audio) {
        SDL_DestroyAudioStream(P.audio);
        P.audio = NULL;
    }
    P.audio_fn = fn;
    P.audio_user = user;
    P.audio_rate = rate;
    spec.format = SDL_AUDIO_S16;
    spec.channels = 2;
    spec.freq = rate;
    P.audio = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                        &spec, audio_cb, NULL);
    if (!P.audio) {
        fprintf(stderr, "plat_audio_open: %s\n", SDL_GetError());
        return -1;
    }
    SDL_ResumeAudioStreamDevice(P.audio);
    return 0;
}

int plat_audio_active(void)
{
    return P.audio != NULL;
}

void plat_audio_lock(void)
{
    if (P.audio)
        SDL_LockAudioStream(P.audio);
}

void plat_audio_unlock(void)
{
    if (P.audio)
        SDL_UnlockAudioStream(P.audio);
}
