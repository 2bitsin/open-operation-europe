/* SDL3 implementation of port/platform/platform.h.
 *
 * Portable C99 + SDL3 only: builds for macOS, Linux, Windows and
 * Emscripten.  Video is an indexed framebuffer converted through the
 * palette into a streaming XRGB8888 texture, shown with SDL's logical
 * presentation (integer scaling + letterbox).  Input is translated to PC
 * set-1 make codes plus the BIOS INT 16h ASCII of a US keyboard.  Files are
 * stdio FILE* handles on paths resolved case-insensitively inside the save
 * directory.  Audio is a pull callback on an SDL_AudioStream.
 *
 * Emscripten (web build, docs/port.md "The browser build"): the game's waits are
 * polling loops that never return to a main loop, so the build uses
 * ASYNCIFY (or JSPI) and this backend hands control back to the browser at
 * exactly two places, both marked __EMSCRIPTEN__: after each presented
 * frame (so the canvas is shown and input events are delivered) and in
 * SleepMs() (the pump's idle sleep).  SDL's own automatic
 * emscripten_sleep() calls are switched off so that these are the only
 * yields.  The page's CSS sizes the canvas to an integer multiple of the
 * frame in device pixels (shell.html); SDL follows that size on window
 * resize events and, with SDL_WINDOW_HIGH_PIXEL_DENSITY, gives the canvas
 * that many device pixels, so the renderer's integer scaling applies there
 * too.  The browser blanks a canvas whose size changes, so a resize or
 * exposure redraws the last frame.  The canvas takes the keyboard events
 * itself and has no vsync setting (the browser paces frames).
 */
#include "platform.hpp"

#include <SDL3/SDL.h>
// The headless --png writer: stb_image_write (conan stb), implemented here, its only user (platform/configure.py
// defines STB_IMAGE_WRITE_IMPLEMENTATION for this file).
#include <stb_image_write.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <print>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace opeu::platform::detail::sdl3 {
namespace {
// true: hide the host cursor while it is over the window (the game draws its own mouse pointer).
constexpr auto HIDE_CURSOR{ true };

// The web shell sizes the canvas to an integer multiple of 640x480 (web/shell.html), so integer scaling applies there
// too.
constexpr auto PRESENTATION{ SDL_LOGICAL_PRESENTATION_INTEGER_SCALE };

// Longest save path, as the DOS-name resolution measures it.
constexpr std::size_t PATH_MAX_LENGTH{ 1024 };
// Longest single component of a DOS name.
constexpr std::size_t COMPONENT_MAX_LENGTH{ 256 };
constexpr std::size_t PALETTE_BYTES{ 256 * 3 };
constexpr int         AUDIO_CHUNK_FRAMES{ 1024 };

// Key kinds for the ASCII computation.
enum class KeyKind : std::uint8_t {
  PLAIN,
  LETTER,
  KEYPAD,
  NOASCII
};

struct KeyMap {
  SDL_Scancode scancode;
  std::uint8_t code;     // set-1 make code
  std::uint8_t extended; // 0xE0 prefix
  KeyKind      kind;
  std::uint8_t lo, hi; // US ASCII unshifted / shifted
  std::uint8_t ctrl;   // ASCII with Ctrl held (0 = none)
};

// clang-format off
constexpr std::array<KeyMap, 105> KEYMAP{ {
  // letters
  { SDL_SCANCODE_A,              0x1E, 0, KeyKind::LETTER,  'a',  'A',  0x01 },
  { SDL_SCANCODE_B,              0x30, 0, KeyKind::LETTER,  'b',  'B',  0x02 },
  { SDL_SCANCODE_C,              0x2E, 0, KeyKind::LETTER,  'c',  'C',  0x03 },
  { SDL_SCANCODE_D,              0x20, 0, KeyKind::LETTER,  'd',  'D',  0x04 },
  { SDL_SCANCODE_E,              0x12, 0, KeyKind::LETTER,  'e',  'E',  0x05 },
  { SDL_SCANCODE_F,              0x21, 0, KeyKind::LETTER,  'f',  'F',  0x06 },
  { SDL_SCANCODE_G,              0x22, 0, KeyKind::LETTER,  'g',  'G',  0x07 },
  { SDL_SCANCODE_H,              0x23, 0, KeyKind::LETTER,  'h',  'H',  0x08 },
  { SDL_SCANCODE_I,              0x17, 0, KeyKind::LETTER,  'i',  'I',  0x09 },
  { SDL_SCANCODE_J,              0x24, 0, KeyKind::LETTER,  'j',  'J',  0x0A },
  { SDL_SCANCODE_K,              0x25, 0, KeyKind::LETTER,  'k',  'K',  0x0B },
  { SDL_SCANCODE_L,              0x26, 0, KeyKind::LETTER,  'l',  'L',  0x0C },
  { SDL_SCANCODE_M,              0x32, 0, KeyKind::LETTER,  'm',  'M',  0x0D },
  { SDL_SCANCODE_N,              0x31, 0, KeyKind::LETTER,  'n',  'N',  0x0E },
  { SDL_SCANCODE_O,              0x18, 0, KeyKind::LETTER,  'o',  'O',  0x0F },
  { SDL_SCANCODE_P,              0x19, 0, KeyKind::LETTER,  'p',  'P',  0x10 },
  { SDL_SCANCODE_Q,              0x10, 0, KeyKind::LETTER,  'q',  'Q',  0x11 },
  { SDL_SCANCODE_R,              0x13, 0, KeyKind::LETTER,  'r',  'R',  0x12 },
  { SDL_SCANCODE_S,              0x1F, 0, KeyKind::LETTER,  's',  'S',  0x13 },
  { SDL_SCANCODE_T,              0x14, 0, KeyKind::LETTER,  't',  'T',  0x14 },
  { SDL_SCANCODE_U,              0x16, 0, KeyKind::LETTER,  'u',  'U',  0x15 },
  { SDL_SCANCODE_V,              0x2F, 0, KeyKind::LETTER,  'v',  'V',  0x16 },
  { SDL_SCANCODE_W,              0x11, 0, KeyKind::LETTER,  'w',  'W',  0x17 },
  { SDL_SCANCODE_X,              0x2D, 0, KeyKind::LETTER,  'x',  'X',  0x18 },
  { SDL_SCANCODE_Y,              0x15, 0, KeyKind::LETTER,  'y',  'Y',  0x19 },
  { SDL_SCANCODE_Z,              0x2C, 0, KeyKind::LETTER,  'z',  'Z',  0x1A },
  // digit row
  { SDL_SCANCODE_1,              0x02, 0, KeyKind::PLAIN,   '1',  '!',  0    },
  { SDL_SCANCODE_2,              0x03, 0, KeyKind::PLAIN,   '2',  '@',  0    },
  { SDL_SCANCODE_3,              0x04, 0, KeyKind::PLAIN,   '3',  '#',  0    },
  { SDL_SCANCODE_4,              0x05, 0, KeyKind::PLAIN,   '4',  '$',  0    },
  { SDL_SCANCODE_5,              0x06, 0, KeyKind::PLAIN,   '5',  '%',  0    },
  { SDL_SCANCODE_6,              0x07, 0, KeyKind::PLAIN,   '6',  '^',  0x1E },
  { SDL_SCANCODE_7,              0x08, 0, KeyKind::PLAIN,   '7',  '&',  0    },
  { SDL_SCANCODE_8,              0x09, 0, KeyKind::PLAIN,   '8',  '*',  0    },
  { SDL_SCANCODE_9,              0x0A, 0, KeyKind::PLAIN,   '9',  '(',  0    },
  { SDL_SCANCODE_0,              0x0B, 0, KeyKind::PLAIN,   '0',  ')',  0    },
  { SDL_SCANCODE_MINUS,          0x0C, 0, KeyKind::PLAIN,   '-',  '_',  0x1F },
  { SDL_SCANCODE_EQUALS,         0x0D, 0, KeyKind::PLAIN,   '=',  '+',  0    },
  { SDL_SCANCODE_BACKSPACE,      0x0E, 0, KeyKind::PLAIN,   0x08, 0x08, 0x7F },
  { SDL_SCANCODE_TAB,            0x0F, 0, KeyKind::PLAIN,   0x09, 0x00, 0    },
  { SDL_SCANCODE_LEFTBRACKET,    0x1A, 0, KeyKind::PLAIN,   '[',  '{',  0x1B },
  { SDL_SCANCODE_RIGHTBRACKET,   0x1B, 0, KeyKind::PLAIN,   ']',  '}',  0x1D },
  { SDL_SCANCODE_RETURN,         0x1C, 0, KeyKind::PLAIN,   0x0D, 0x0D, 0x0A },
  { SDL_SCANCODE_SEMICOLON,      0x27, 0, KeyKind::PLAIN,   ';',  ':',  0    },
  { SDL_SCANCODE_APOSTROPHE,     0x28, 0, KeyKind::PLAIN,   '\'', '"',  0    },
  { SDL_SCANCODE_GRAVE,          0x29, 0, KeyKind::PLAIN,   '`',  '~',  0    },
  { SDL_SCANCODE_BACKSLASH,      0x2B, 0, KeyKind::PLAIN,   '\\', '|',  0x1C },
  { SDL_SCANCODE_NONUSBACKSLASH, 0x56, 0, KeyKind::PLAIN,   '\\', '|',  0x1C },
  { SDL_SCANCODE_COMMA,          0x33, 0, KeyKind::PLAIN,   ',',  '<',  0    },
  { SDL_SCANCODE_PERIOD,         0x34, 0, KeyKind::PLAIN,   '.',  '>',  0    },
  { SDL_SCANCODE_SLASH,          0x35, 0, KeyKind::PLAIN,   '/',  '?',  0    },
  { SDL_SCANCODE_SPACE,          0x39, 0, KeyKind::PLAIN,   ' ',  ' ',  ' '  },
  { SDL_SCANCODE_ESCAPE,         0x01, 0, KeyKind::PLAIN,   0x1B, 0x1B, 0x1B },
  // modifiers and locks
  { SDL_SCANCODE_LCTRL,          0x1D, 0, KeyKind::NOASCII, 0,    0,    0    },
  { SDL_SCANCODE_RCTRL,          0x1D, 1, KeyKind::NOASCII, 0,    0,    0    },
  { SDL_SCANCODE_LSHIFT,         0x2A, 0, KeyKind::NOASCII, 0,    0,    0    },
  { SDL_SCANCODE_RSHIFT,         0x36, 0, KeyKind::NOASCII, 0,    0,    0    },
  { SDL_SCANCODE_LALT,           0x38, 0, KeyKind::NOASCII, 0,    0,    0    },
  { SDL_SCANCODE_RALT,           0x38, 1, KeyKind::NOASCII, 0,    0,    0    },
  { SDL_SCANCODE_LGUI,           0x5B, 1, KeyKind::NOASCII, 0,    0,    0    },
  { SDL_SCANCODE_RGUI,           0x5C, 1, KeyKind::NOASCII, 0,    0,    0    },
  { SDL_SCANCODE_APPLICATION,    0x5D, 1, KeyKind::NOASCII, 0,    0,    0    },
  { SDL_SCANCODE_CAPSLOCK,       0x3A, 0, KeyKind::NOASCII, 0,    0,    0    },
  { SDL_SCANCODE_NUMLOCKCLEAR,   0x45, 0, KeyKind::NOASCII, 0,    0,    0    },
  { SDL_SCANCODE_SCROLLLOCK,     0x46, 0, KeyKind::NOASCII, 0,    0,    0    },
  { SDL_SCANCODE_PRINTSCREEN,    0x37, 1, KeyKind::NOASCII, 0,    0,    0    },
  // function keys
  { SDL_SCANCODE_F1,             0x3B, 0, KeyKind::NOASCII, 0,    0,    0    },
  { SDL_SCANCODE_F2,             0x3C, 0, KeyKind::NOASCII, 0,    0,    0    },
  { SDL_SCANCODE_F3,             0x3D, 0, KeyKind::NOASCII, 0,    0,    0    },
  { SDL_SCANCODE_F4,             0x3E, 0, KeyKind::NOASCII, 0,    0,    0    },
  { SDL_SCANCODE_F5,             0x3F, 0, KeyKind::NOASCII, 0,    0,    0    },
  { SDL_SCANCODE_F6,             0x40, 0, KeyKind::NOASCII, 0,    0,    0    },
  { SDL_SCANCODE_F7,             0x41, 0, KeyKind::NOASCII, 0,    0,    0    },
  { SDL_SCANCODE_F8,             0x42, 0, KeyKind::NOASCII, 0,    0,    0    },
  { SDL_SCANCODE_F9,             0x43, 0, KeyKind::NOASCII, 0,    0,    0    },
  { SDL_SCANCODE_F10,            0x44, 0, KeyKind::NOASCII, 0,    0,    0    },
  { SDL_SCANCODE_F11,            0x57, 0, KeyKind::NOASCII, 0,    0,    0    },
  { SDL_SCANCODE_F12,            0x58, 0, KeyKind::NOASCII, 0,    0,    0    },
  // grey navigation block (0xE0 prefixed)
  { SDL_SCANCODE_INSERT,         0x52, 1, KeyKind::NOASCII, 0,    0,    0    },
  { SDL_SCANCODE_DELETE,         0x53, 1, KeyKind::NOASCII, 0,    0,    0    },
  { SDL_SCANCODE_HOME,           0x47, 1, KeyKind::NOASCII, 0,    0,    0    },
  { SDL_SCANCODE_END,            0x4F, 1, KeyKind::NOASCII, 0,    0,    0    },
  { SDL_SCANCODE_PAGEUP,         0x49, 1, KeyKind::NOASCII, 0,    0,    0    },
  { SDL_SCANCODE_PAGEDOWN,       0x51, 1, KeyKind::NOASCII, 0,    0,    0    },
  { SDL_SCANCODE_UP,             0x48, 1, KeyKind::NOASCII, 0,    0,    0    },
  { SDL_SCANCODE_DOWN,           0x50, 1, KeyKind::NOASCII, 0,    0,    0    },
  { SDL_SCANCODE_LEFT,           0x4B, 1, KeyKind::NOASCII, 0,    0,    0    },
  { SDL_SCANCODE_RIGHT,          0x4D, 1, KeyKind::NOASCII, 0,    0,    0    },
  // keypad
  { SDL_SCANCODE_KP_DIVIDE,      0x35, 1, KeyKind::PLAIN,   '/',  '/',  0    },
  { SDL_SCANCODE_KP_MULTIPLY,    0x37, 0, KeyKind::PLAIN,   '*',  '*',  0    },
  { SDL_SCANCODE_KP_MINUS,       0x4A, 0, KeyKind::PLAIN,   '-',  '-',  0    },
  { SDL_SCANCODE_KP_PLUS,        0x4E, 0, KeyKind::PLAIN,   '+',  '+',  0    },
  { SDL_SCANCODE_KP_ENTER,       0x1C, 1, KeyKind::PLAIN,   0x0D, 0x0D, 0x0A },
  { SDL_SCANCODE_KP_EQUALS,      0x0D, 0, KeyKind::PLAIN,   '=',  '=',  0    },
  { SDL_SCANCODE_KP_7,           0x47, 0, KeyKind::KEYPAD,  '7',  0,    0    },
  { SDL_SCANCODE_KP_8,           0x48, 0, KeyKind::KEYPAD,  '8',  0,    0    },
  { SDL_SCANCODE_KP_9,           0x49, 0, KeyKind::KEYPAD,  '9',  0,    0    },
  { SDL_SCANCODE_KP_4,           0x4B, 0, KeyKind::KEYPAD,  '4',  0,    0    },
  { SDL_SCANCODE_KP_5,           0x4C, 0, KeyKind::KEYPAD,  '5',  0,    0    },
  { SDL_SCANCODE_KP_6,           0x4D, 0, KeyKind::KEYPAD,  '6',  0,    0    },
  { SDL_SCANCODE_KP_1,           0x4F, 0, KeyKind::KEYPAD,  '1',  0,    0    },
  { SDL_SCANCODE_KP_2,           0x50, 0, KeyKind::KEYPAD,  '2',  0,    0    },
  { SDL_SCANCODE_KP_3,           0x51, 0, KeyKind::KEYPAD,  '3',  0,    0    },
  { SDL_SCANCODE_KP_0,           0x52, 0, KeyKind::KEYPAD,  '0',  0,    0    },
  { SDL_SCANCODE_KP_PERIOD,      0x53, 0, KeyKind::KEYPAD,  '.',  0,    0    },
} };
// clang-format on

auto FindKey(SDL_Scancode scancode) -> KeyMap const* {
  auto const found{ std::ranges::find(KEYMAP, scancode, &KeyMap::scancode) };
  return found == KEYMAP.end() ? nullptr : &*found;
}

// BIOS INT 16h ASCII of a key for the given SDL modifier state.
auto KeyAscii(KeyMap const& key, SDL_Keymod mod) -> std::uint8_t {
  bool shift{ (mod & SDL_KMOD_SHIFT) != 0 };
  if (key.kind == KeyKind::NOASCII) return 0;
  if ((mod & SDL_KMOD_ALT) != 0) return 0; // Alt combinations: ASCII 00
  if ((mod & SDL_KMOD_CTRL) != 0) return key.ctrl;
  switch (key.kind) {
  case KeyKind::LETTER:
    if ((mod & SDL_KMOD_CAPS) != 0) shift = !shift;
    return shift ? key.hi : key.lo;
  case KeyKind::KEYPAD:
    // Digits only when Num Lock xor Shift; otherwise cursor keys.
    return ((mod & SDL_KMOD_NUM) != 0) != shift ? key.lo : 0;
  default: return shift ? key.hi : key.lo;
  }
}

auto Uppercase(char c) -> char {
  return static_cast<char>(SDL_toupper(static_cast<unsigned char>(c)));
}

auto IsSeparator(char c) -> bool {
  return c == '\\' || c == '/';
}

auto PathExists(std::string const& path) -> bool {
  return SDL_GetPathInfo(path.c_str(), nullptr);
}

// A directory search for one name, case-insensitively; an exact spelling wins.
struct FindContext {
  std::string want;
  std::string found;
  bool        ok{ false };
};

auto SDLCALL FindEntry(void* context, [[maybe_unused]] char const* directory, char const* name) -> SDL_EnumerationResult {
  auto& find{ *static_cast<FindContext*>(context) };
  if (SDL_strcasecmp(name, find.want.c_str()) == 0) {
    find.found = name;
    find.ok    = true;
    if (find.want == name) return SDL_ENUM_SUCCESS;
  }
  return SDL_ENUM_CONTINUE;
}

// The SDL window, renderer, audio stream and the last frame, behind the plat_* contract.
class Backend {
public:
  auto               Init(PlatConfig const& config) -> int;
  auto               Shutdown() -> void;
  auto               Present(std::span<std::uint8_t const> pixels, int width, int height, std::span<std::uint8_t const, PALETTE_BYTES> palette) -> void;
  auto               WritePng(char const* path) const -> int;
  auto               PollEvent(PlatEvent& event) -> int;
  [[nodiscard]] auto ShiftFlags() const -> std::uint8_t;
  [[nodiscard]] auto TicksUs() const -> std::uint64_t;
  [[nodiscard]] auto Headless() const -> bool {
    return _headless;
  }
  auto               SetSaveDir(std::string_view dir) -> void;
  auto               SaveDir() -> std::string const&;
  auto               Resolve(std::string_view dos_name, bool create) -> std::pair<int, std::string>;
  auto               OpenAudio(int rate, PlatAudioFn callback, void* user) -> int;
  [[nodiscard]] auto AudioActive() const -> bool {
    return _audio != nullptr;
  }
  auto LockAudio() const -> void;
  auto UnlockAudio() const -> void;

private:
  auto                OpenWindow(PlatConfig const& config, int width, int height, int scale) -> int;
  auto                Redraw() const -> void;
  auto                MousePosition(float fx, float fy, PlatEvent& event) const -> void;
  static auto SDLCALL FillAudio(void* backend, SDL_AudioStream* stream, int additional, int total) -> void;

  bool          _inited{ false };
  bool          _headless{ false };
  SDL_InitFlags _sdl_flags{ 0 };
  std::uint64_t _t0_ns{ 0 };

  SDL_Window*   _window{ nullptr };
  SDL_Renderer* _renderer{ nullptr };
  SDL_Texture*  _texture{ nullptr };
  int           _texture_width{ 0 }, _texture_height{ 0 };

  std::vector<std::uint8_t>               _frame; // the last indexed frame
  int                                     _frame_width{ 0 }, _frame_height{ 0 };
  std::array<std::uint8_t, PALETTE_BYTES> _palette{};
  std::vector<std::uint32_t>              _row; // one converted texture row

  SDL_AudioStream* _audio{ nullptr };
  PlatAudioFn      _audio_callback{ nullptr };
  void*            _audio_user{ nullptr };
  int              _audio_rate{ 0 };

  std::string _save_dir;
};

auto TheBackend() -> Backend& {
  static Backend backend;
  return backend;
}

// ---- lifecycle ----------------------------------------------------------------------------------------------------

auto Backend::Init(PlatConfig const& config) -> int {
  if (_inited) return 0;
  _headless = config.headless != 0;
  int const width{ config.width > 0 ? config.width : 640 };
  int const height{ config.height > 0 ? config.height : 480 };
  int const scale{ std::max(config.scale, 1) };

  if (_headless) {
    // No subsystems: timers and the filesystem helpers need none.
    if (!SDL_Init(0)) {
      std::println(stderr, "plat_init: SDL_Init: {}", SDL_GetError());
      return -1;
    }
    _sdl_flags = 0;
  } else if (OpenWindow(config, width, height, scale) != 0) {
    return -1;
  }
  _t0_ns  = SDL_GetTicksNS();
  _inited = true;
  return 0;
}

auto Backend::OpenWindow(PlatConfig const& config, int width, int height, int scale) -> int {
  char const* const title{ config.title != nullptr ? config.title : "Operation Europe" };
  SDL_SetAppMetadata(title, nullptr, nullptr);
#ifdef __EMSCRIPTEN__
  // Keys go to the canvas (focused by the page's click-to-start), and only this backend yields to the browser (see the
  // file comment).
  SDL_SetHint(SDL_HINT_EMSCRIPTEN_KEYBOARD_ELEMENT, "#canvas");
  SDL_SetHint(SDL_HINT_EMSCRIPTEN_ASYNCIFY, "0");
#endif
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
    std::println(stderr, "plat_init: SDL_Init(video): {}", SDL_GetError());
    return -1;
  }
  _sdl_flags = SDL_INIT_VIDEO | SDL_INIT_EVENTS;
  // Audio is optional: continue silently if there is no device.
  if (SDL_InitSubSystem(SDL_INIT_AUDIO))
    _sdl_flags |= SDL_INIT_AUDIO;
  else
    std::println(stderr, "plat_init: no audio: {}", SDL_GetError());

  SDL_WindowFlags window_flags{ SDL_WINDOW_RESIZABLE };
  if (config.fullscreen != 0) window_flags |= SDL_WINDOW_FULLSCREEN;
#ifdef __EMSCRIPTEN__
  // The canvas gets CSS size x devicePixelRatio pixels. Without this the backing store is CSS pixels: on a 2x or 3x
  // display the page's integer multiple of 640x480 device pixels is then a fraction of the frame, and the integer-scale
  // presentation crops it. The WebGL renderer needs an OpenGL window; given the flag up front, it does not recreate the
  // window, which would leave the canvas's CSS size at 1px (SDL's destroy) and make the new window ignore the page's
  // size.
  window_flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_OPENGL;
#endif
  if (!SDL_CreateWindowAndRenderer(title, width * scale, height * scale, window_flags, &_window, &_renderer)) {
    std::println(stderr, "plat_init: SDL_CreateWindowAndRenderer: {}", SDL_GetError());
    if (_window != nullptr) SDL_DestroyWindow(_window);
    _window   = nullptr;
    _renderer = nullptr;
    SDL_Quit();
    return -1;
  }
#ifndef __EMSCRIPTEN__
  SDL_SetRenderVSync(_renderer, 1);
#endif
  SDL_SetRenderLogicalPresentation(_renderer, width, height, PRESENTATION);
  if constexpr (HIDE_CURSOR) SDL_HideCursor();
  // Never deliver text-input events; the backend derives ASCII itself.
  SDL_StopTextInput(_window);
  SDL_SetRenderDrawColor(_renderer, 0, 0, 0, 255);
  SDL_RenderClear(_renderer);
  SDL_RenderPresent(_renderer);
  return 0;
}

auto Backend::Shutdown() -> void {
  if (!_inited) return;
  if (_audio != nullptr) {
    SDL_DestroyAudioStream(_audio);
    _audio = nullptr;
  }
  if (_texture != nullptr) {
    SDL_DestroyTexture(_texture);
    _texture = nullptr;
  }
  if (_renderer != nullptr) {
    SDL_DestroyRenderer(_renderer);
    _renderer = nullptr;
  }
  if (_window != nullptr) {
    SDL_DestroyWindow(_window);
    _window = nullptr;
  }
  _frame.clear();
  SDL_Quit();
  _inited = false;
}

// ---- video --------------------------------------------------------------------------------------------------------

// Show the last converted frame again: after each present, and after a window resize or exposure, when the window's
// contents are gone (a browser blanks a canvas whose size changed) and the game, waiting for input, may not draw again
// for a long time.
auto Backend::Redraw() const -> void {
  if (_renderer == nullptr || _texture == nullptr) return;
  SDL_SetRenderDrawColor(_renderer, 0, 0, 0, 255);
  SDL_RenderClear(_renderer);
  SDL_RenderTexture(_renderer, _texture, nullptr, nullptr);
  SDL_RenderPresent(_renderer);
}

auto Backend::Present(std::span<std::uint8_t const> pixels, int width, int height, std::span<std::uint8_t const, PALETTE_BYTES> palette) -> void {
  _frame.assign(pixels.begin(), pixels.end());
  _frame_width  = width;
  _frame_height = height;
  std::ranges::copy(palette, _palette.begin());

  if (_renderer == nullptr) return;

  if (_texture == nullptr || _texture_width != width || _texture_height != height) {
    if (_texture != nullptr) SDL_DestroyTexture(_texture);
    _texture = SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
    if (_texture == nullptr) return;
    SDL_SetTextureScaleMode(_texture, SDL_SCALEMODE_NEAREST);
    _texture_width  = width;
    _texture_height = height;
    SDL_SetRenderLogicalPresentation(_renderer, width, height, PRESENTATION);
  }

  std::array<std::uint32_t, 256> lut{};
  std::ranges::transform(std::views::iota(0uz, lut.size()), lut.begin(), [&palette](std::size_t index) -> std::uint32_t {
    std::array<std::uint32_t, 3> rgb{};
    std::ranges::copy(palette.subspan(index * 3, 3), rgb.begin());
    auto const [red, green, blue]{ rgb };
    return 0xFF000000u | red << 16 | green << 8 | blue;
  });

  void* locked{ nullptr };
  int   pitch{ 0 };
  if (SDL_LockTexture(_texture, nullptr, &locked, &pitch)) {
    auto const      row_bytes{ static_cast<std::size_t>(width) * sizeof(std::uint32_t) };
    std::span const texture{ static_cast<std::byte*>(locked), static_cast<std::size_t>(pitch) * static_cast<std::size_t>(height) };
    _row.resize(static_cast<std::size_t>(width));
    auto const columns{ static_cast<std::size_t>(width) };
    for (auto const y : std::views::iota(0uz, static_cast<std::size_t>(height))) {
      std::ranges::transform(pixels.subspan(y * columns, columns), _row.begin(), [&lut](std::uint8_t index) -> std::uint32_t { return lut.at(index); });
      std::memcpy(texture.subspan(y * static_cast<std::size_t>(pitch)).data(), _row.data(), row_bytes);
    }
    SDL_UnlockTexture(_texture);
  }
  Redraw();
#ifdef __EMSCRIPTEN__
  // Return to the browser so it shows the frame and queues input.
  emscripten_sleep(0);
#endif
}

auto Backend::WritePng(char const* path) const -> int {
  if (_frame.empty() || path == nullptr) return -1;
  std::vector<std::uint8_t> rgb;
  rgb.reserve(_frame.size() * 3);
  for (std::uint8_t const index : _frame) {
    auto const colour{ std::span{ _palette }.subspan(static_cast<std::size_t>(index) * 3, 3) };
    rgb.insert(rgb.end(), colour.begin(), colour.end());
  }
  // 8-bit truecolour, filter type 0 on every row, so the image readers of the integration suites need no unfiltering.
  stbi_write_force_png_filter = 0;
  return stbi_write_png(path, _frame_width, _frame_height, 3, rgb.data(), _frame_width * 3) != 0 ? 0 : -1;
}

// ---- input --------------------------------------------------------------------------------------------------------

auto Backend::MousePosition(float fx, float fy, PlatEvent& event) const -> void {
  auto const extent{ [](int texture, int frame, int fallback) -> int {
    if (texture > 0) return texture;
    return frame > 0 ? frame : fallback;
  } };
  int const width{ extent(_texture_width, _frame_width, 640) };
  int const height{ extent(_texture_height, _frame_height, 480) };
  event.x = std::clamp(static_cast<int>(SDL_floorf(fx)), 0, width - 1);
  event.y = std::clamp(static_cast<int>(SDL_floorf(fy)), 0, height - 1);
}

auto Backend::PollEvent(PlatEvent& event) -> int {
  if (!_inited || _headless) return 0;
  SDL_Event sdl{};
  while (SDL_PollEvent(&sdl)) {
    event = PlatEvent{};
    switch (sdl.type) {
    case SDL_EVENT_QUIT:                      event.type = PLAT_EV_QUIT; return 1;
    case SDL_EVENT_WINDOW_RESIZED:
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
    case SDL_EVENT_WINDOW_EXPOSED:            Redraw(); break;
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:                    {
      // SDL key repeat arrives as further KEY_DOWN events (repeat=1), which is exactly the DOS typematic behaviour.
      KeyMap const* const key{ FindKey(sdl.key.scancode) };
      if (key == nullptr) break;
      event.type     = sdl.type == SDL_EVENT_KEY_DOWN ? PLAT_EV_KEY_DOWN : PLAT_EV_KEY_UP;
      event.scancode = key->code;
      event.extended = key->extended;
      event.ascii    = event.type == PLAT_EV_KEY_DOWN ? KeyAscii(*key, sdl.key.mod) : 0;
      return 1;
    }
    case SDL_EVENT_MOUSE_MOTION:
      SDL_ConvertEventToRenderCoordinates(_renderer, &sdl);
      event.type = PLAT_EV_MOUSE_MOVE;
      MousePosition(sdl.motion.x, sdl.motion.y, event);
      return 1;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:   {
      std::uint8_t button{ 0 };
      if (sdl.button.button == SDL_BUTTON_LEFT)
        button = 0;
      else if (sdl.button.button == SDL_BUTTON_RIGHT)
        button = 1;
      else if (sdl.button.button == SDL_BUTTON_MIDDLE)
        button = 2;
      else
        break;
      SDL_ConvertEventToRenderCoordinates(_renderer, &sdl);
      event.type   = sdl.type == SDL_EVENT_MOUSE_BUTTON_DOWN ? PLAT_EV_MOUSE_DOWN : PLAT_EV_MOUSE_UP;
      event.button = button;
      MousePosition(sdl.button.x, sdl.button.y, event);
      return 1;
    }
    default: break;
    }
  }
  event = PlatEvent{};
  return 0;
}

auto Backend::ShiftFlags() const -> std::uint8_t {
  if (!_inited || _headless) return 0;
  SDL_Keymod const mod{ SDL_GetModState() };
  // clang-format off
  constexpr std::array<std::pair<SDL_Keymod, std::uint8_t>, 7> FLAGS{ {
    { SDL_KMOD_RSHIFT, 0x01 },
    { SDL_KMOD_LSHIFT, 0x02 },
    { SDL_KMOD_CTRL,   0x04 },
    { SDL_KMOD_ALT,    0x08 },
    { SDL_KMOD_SCROLL, 0x10 },
    { SDL_KMOD_NUM,    0x20 },
    { SDL_KMOD_CAPS,   0x40 },
  } };
  // clang-format on
  std::uint8_t flags{ 0 };
  for (auto const& [modifier, bit] : FLAGS)
    if ((mod & modifier) != 0) flags |= bit;
  return flags;
}

// ---- time ---------------------------------------------------------------------------------------------------------

auto Backend::TicksUs() const -> std::uint64_t {
  return (SDL_GetTicksNS() - _t0_ns) / 1000u;
}

// ---- files --------------------------------------------------------------------------------------------------------

auto Backend::SetSaveDir(std::string_view dir) -> void {
  if (dir.empty()) dir = ".";
  _save_dir = dir.substr(0, PATH_MAX_LENGTH - 1);
  while (_save_dir.size() > 1 && IsSeparator(_save_dir.back())) _save_dir.pop_back();
#ifndef __EMSCRIPTEN__
  SDL_CreateDirectory(_save_dir.c_str()); // a first run's --saves DIR
#endif
}

auto Backend::SaveDir() -> std::string const& {
  if (_save_dir.empty()) {
#ifdef __EMSCRIPTEN__
    SetSaveDir("/saves"); // web/pre.js mounts IDBFS there
#else
    char* const pref{ SDL_GetPrefPath("opeu", "Operation Europe") };
    SetSaveDir(pref != nullptr ? pref : ".");
    SDL_free(pref);
#endif
  }
  return _save_dir;
}

// Resolves a DOS name (with '\' or '/' separators) inside the save directory, matching every component
// case-insensitively. When the final component is missing and `create` is set, it is appended in upper case (the DOS
// spelling). The result is 1 if the file exists, 0 if a creatable path was produced, -1 on failure, with the path.
auto Backend::Resolve(std::string_view dos_name, bool create) -> std::pair<int, std::string> {
  if (dos_name.empty()) return { -1, {} };
  std::string path{ SaveDir() };
  auto const  is_part{ [](char c) -> bool { return !IsSeparator(c); } };
  auto        rest{ dos_name };
  while (!rest.empty()) {
    rest.remove_prefix(std::min(rest.find_first_not_of("\\/"), rest.size()));
    if (rest.empty()) break;
    auto const             length{ static_cast<std::size_t>(std::ranges::find_if_not(rest, is_part) - rest.begin()) };
    std::string_view const component{ rest.substr(0, length) };
    if (length >= COMPONENT_MAX_LENGTH) return { -1, {} };
    rest.remove_prefix(length);
    bool const last{ rest.find_first_not_of("\\/") == std::string_view::npos };
    if (path.size() + 1 + length + 1 > PATH_MAX_LENGTH) return { -1, {} };
    // Fast path: the exact spelling exists.
    std::string const exact{ path + "/" + std::string{ component } };
    if (PathExists(exact)) {
      path = exact;
      continue;
    }
    FindContext find{ .want = std::string{ component }, .found = {}, .ok = false };
    SDL_EnumerateDirectory(path.c_str(), FindEntry, &find);
    if (find.ok) {
      if (path.size() + 1 + find.found.size() + 1 > PATH_MAX_LENGTH) return { -1, {} };
      path += "/" + find.found;
      continue;
    }
    if (!last || !create) return { -1, {} };
    path += '/';
    std::ranges::transform(component, std::back_inserter(path), Uppercase);
    return { 0, path };
  }
  return { 1, path };
}

// ---- audio --------------------------------------------------------------------------------------------------------

auto SDLCALL Backend::FillAudio(void* backend, SDL_AudioStream* stream, int additional, [[maybe_unused]] int total) -> void {
  auto const&                                                                self{ *static_cast<Backend const*>(backend) };
  std::array<std::int16_t, static_cast<std::size_t>(AUDIO_CHUNK_FRAMES) * 2> samples{};
  // `additional` is in bytes; one frame is 4 bytes (S16 stereo).
  while (additional >= 4) {
    int const frames{ std::min(additional / 4, AUDIO_CHUNK_FRAMES) };
    if (self._audio_callback != nullptr)
      self._audio_callback(self._audio_user, samples.data(), frames, self._audio_rate);
    else
      std::ranges::fill(samples, std::int16_t{ 0 });
    SDL_PutAudioStreamData(stream, samples.data(), frames * 4);
    additional -= frames * 4;
  }
}

auto Backend::OpenAudio(int rate, PlatAudioFn callback, void* user) -> int {
  if (!_inited || _headless) return 0;
  if ((_sdl_flags & SDL_INIT_AUDIO) == 0) return -1;
  if (rate <= 0) rate = 44100;
  if (_audio != nullptr) {
    SDL_DestroyAudioStream(_audio);
    _audio = nullptr;
  }
  _audio_callback = callback;
  _audio_user     = user;
  _audio_rate     = rate;
  SDL_AudioSpec const spec{ .format = SDL_AUDIO_S16, .channels = 2, .freq = rate };
  _audio = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, FillAudio, this);
  if (_audio == nullptr) {
    std::println(stderr, "plat_audio_open: {}", SDL_GetError());
    return -1;
  }
  SDL_ResumeAudioStreamDevice(_audio);
  return 0;
}

auto Backend::LockAudio() const -> void {
  if (_audio != nullptr) SDL_LockAudioStream(_audio);
}

auto Backend::UnlockAudio() const -> void {
  if (_audio != nullptr) SDL_UnlockAudioStream(_audio);
}

auto AsFile(void* handle) -> std::FILE* {
  return static_cast<std::FILE*>(handle);
}
} // namespace
} // namespace opeu::platform::detail::sdl3

// ---- the platform contract ----------------------------------------------------------------------------------------

using opeu::platform::detail::sdl3::AsFile;
using opeu::platform::detail::sdl3::PALETTE_BYTES;
using opeu::platform::detail::sdl3::TheBackend;

auto plat_init(PlatConfig const* cfg) -> int {
  return cfg == nullptr ? -1 : TheBackend().Init(*cfg);
}

auto plat_shutdown() -> void {
  TheBackend().Shutdown();
}

auto plat_present(std::uint8_t const* pixels, int width, int height, std::uint8_t const* palette) -> void {
  if (pixels == nullptr || palette == nullptr || width <= 0 || height <= 0) return;
  auto const count{ static_cast<std::size_t>(width) * static_cast<std::size_t>(height) };
  TheBackend().Present({ pixels, count }, width, height, std::span<std::uint8_t const, PALETTE_BYTES>{ palette, PALETTE_BYTES });
}

auto plat_write_png(char const* path) -> int {
  return TheBackend().WritePng(path);
}

auto PollEvent(PlatEvent* ev) -> int {
  return ev == nullptr ? 0 : TheBackend().PollEvent(*ev);
}

auto ShiftFlags() -> std::uint8_t {
  return TheBackend().ShiftFlags();
}

auto TicksUs() -> std::uint64_t {
  return TheBackend().TicksUs();
}

auto SleepMs(int ms) -> void {
#ifdef __EMSCRIPTEN__
  // Yield to the browser event loop (ASYNCIFY/JSPI unwinds the stack and resumes here from a timer).
  if (ms >= 0 && !TheBackend().Headless()) emscripten_sleep(static_cast<unsigned>(ms));
#else
  if (ms > 0) SDL_Delay(static_cast<Uint32>(ms));
#endif
}

auto plat_set_save_dir(char const* dir) -> void {
  TheBackend().SetSaveDir(dir != nullptr ? dir : "");
}

auto SaveDir() -> char const* {
  return TheBackend().SaveDir().c_str();
}

auto Fopen(char const* dos_name, char const* mode) -> void* {
  if (dos_name == nullptr || mode == nullptr) return nullptr;
  std::string_view const how{ mode };
  bool const             creating{ how.contains('w') || how.contains('a') };
  auto const [found, path]{ TheBackend().Resolve(dos_name, creating) };
  if (found < 0 || (found == 0 && !creating)) return nullptr;
  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory): the handle crosses the contract as void*; Fclose frees it
  return std::fopen(path.c_str(), mode);
}

auto Fclose(void* f) -> int {
  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory): releases the handle Fopen returned
  return f != nullptr ? std::fclose(AsFile(f)) : -1;
}

auto Fread(void* buf, std::size_t n, void* f) -> std::size_t {
  if (f == nullptr || buf == nullptr || n == 0) return 0;
  return std::fread(buf, 1, n, AsFile(f));
}

auto Fwrite(void const* buf, std::size_t n, void* f) -> std::size_t {
  if (f == nullptr || buf == nullptr || n == 0) return 0;
  return std::fwrite(buf, 1, n, AsFile(f));
}

auto Fseek(void* f, long off, int whence) -> long {
  if (f == nullptr) return -1;
  if (std::fseek(AsFile(f), off, whence) != 0) return -1;
  return std::ftell(AsFile(f));
}

auto Fsize(void* f) -> long {
  if (f == nullptr) return -1;
  long const at{ std::ftell(AsFile(f)) };
  if (at < 0 || std::fseek(AsFile(f), 0, SEEK_END) != 0) return -1;
  long const end{ std::ftell(AsFile(f)) };
  std::fseek(AsFile(f), at, SEEK_SET);
  return end;
}

auto Fexists(char const* dos_name) -> int {
  return dos_name != nullptr && TheBackend().Resolve(dos_name, false).first == 1 ? 1 : 0;
}

auto Fdelete(char const* dos_name) -> int {
  if (dos_name == nullptr) return -1;
  auto const [found, path]{ TheBackend().Resolve(dos_name, false) };
  if (found != 1) return -1;
  return std::remove(path.c_str()) == 0 ? 0 : -1;
}

// DOS semantics: fails if the source is missing or the target exists.
auto Frename(char const* from, char const* to) -> int {
  if (from == nullptr || to == nullptr) return -1;
  auto const [source_found, source]{ TheBackend().Resolve(from, false) };
  if (source_found != 1) return -1;
  auto const [target_found, target]{ TheBackend().Resolve(to, true) };
  if (target_found != 0) return -1;
  return std::rename(source.c_str(), target.c_str()) == 0 ? 0 : -1;
}

auto AudioOpen(int rate, PlatAudioFn fn, void* user) -> int {
  return TheBackend().OpenAudio(rate, fn, user);
}

auto AudioActive() -> int {
  return TheBackend().AudioActive() ? 1 : 0;
}

auto AudioLock() -> void {
  TheBackend().LockAudio();
}

auto AudioUnlock() -> void {
  TheBackend().UnlockAudio();
}
