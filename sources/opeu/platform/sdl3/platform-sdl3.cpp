// census: raii — SDL's handles live in their owners (handles.hpp); the raw pointers left are SDL's callback contexts
// and out-parameters, wrapped where SDL hands them over.
//
// The SDL3 implementation of platform.hpp, for macOS, Linux, Windows and the browser. Video is the indexed frame
// converted through the palette into a streaming XRGB8888 texture, shown with SDL's logical presentation (integer
// scaling and letterbox). Input becomes PC set-1 make codes with the BIOS INT 16h ASCII of a US keyboard. Save files
// are streams on paths resolved case-insensitively inside the save directory. Audio is a pull callback on an
// SDL_AudioStream.
//
// In the browser (docs/port.md "The browser build") the game's waits are polling loops that never return to a main
// loop, so the build uses ASYNCIFY (or JSPI) and this backend hands control back to the browser at exactly two places,
// both marked __EMSCRIPTEN__: after each presented frame (so the canvas is shown and input events are delivered) and
// in SleepMs (the pump's idle sleep). SDL's own automatic emscripten_sleep() calls are switched off, so these are the
// only yields. The page's CSS sizes the canvas to an integer multiple of the frame in device pixels (shell.html); SDL
// follows that size on window resize events and, with SDL_WINDOW_HIGH_PIXEL_DENSITY, gives the canvas that many
// device pixels, so the renderer's integer scaling applies there too. The browser blanks a canvas whose size changes,
// so a resize or exposure redraws the last frame. The canvas takes the keyboard events itself and has no vsync
// setting (the browser paces frames).
#include "opeu/platform/platform.hpp"

#include "opeu/platform/sdl3/handles.hpp"

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
constexpr int         AUDIO_CHUNK_FRAMES{ 1024 };
constexpr int         DEFAULT_WIDTH{ 640 };
constexpr int         DEFAULT_HEIGHT{ 480 };
constexpr int         DEFAULT_AUDIO_RATE{ 44100 };
constexpr auto        DEFAULT_TITLE{ std::string_view{ "Operation Europe" } };

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

auto FindKey(SDL_Scancode const scancode) -> std::optional<KeyMap> {
  auto const found{ std::ranges::find(KEYMAP, scancode, &KeyMap::scancode) };
  if (found == KEYMAP.end()) return std::nullopt;
  return *found;
}

// BIOS INT 16h ASCII of a key for the given SDL modifier state.
auto KeyAscii(KeyMap const& key, SDL_Keymod const mod) -> std::uint8_t {
  auto shift{ (mod & SDL_KMOD_SHIFT) != 0 };
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

// An event of the given type, its other fields zero until set.
auto Event(EventType const type) -> InputEvent {
  InputEvent event{};
  event.type = type;
  return event;
}

// The button number the port reports: 0 left, 1 right, 2 middle; nothing for the others.
auto ButtonNumber(Uint8 const button) -> std::optional<std::uint8_t> {
  switch (button) {
  case SDL_BUTTON_LEFT:   return 0;
  case SDL_BUTTON_RIGHT:  return 1;
  case SDL_BUTTON_MIDDLE: return 2;
  default:                return std::nullopt;
  }
}

auto Uppercase(char const c) -> char {
  return static_cast<char>(SDL_toupper(static_cast<unsigned char>(c)));
}

auto IsSeparator(char const c) -> bool {
  return c == '\\' || c == '/';
}

auto PathExists(std::string const& path) -> bool {
  return SDL_GetPathInfo(path.c_str(), nullptr);
}

// A directory search for one name, case-insensitively; an exact spelling wins.
struct FindContext {
  std::string                want;
  std::optional<std::string> found;
};

auto SDLCALL FindEntry(void* const context, [[maybe_unused]] char const* const directory, char const* const name) -> SDL_EnumerationResult {
  auto&                  find{ *static_cast<FindContext*>(context) };
  std::string_view const entry{ name };
  if (SDL_strcasecmp(name, find.want.c_str()) == 0) {
    find.found = std::string{ entry };
    if (find.want == entry) return SDL_ENUM_SUCCESS;
  }
  return SDL_ENUM_CONTINUE;
}

auto Spelled(std::string const& directory, std::string const& want) -> std::optional<std::string> {
  FindContext find{ .want = want, .found = std::nullopt };
  SDL_EnumerateDirectory(directory.c_str(), FindEntry, &find);
  return find.found;
}

// Where a DOS name leads inside the save directory.
enum class Found : std::uint8_t {
  EXISTS,    // every component exists
  CREATABLE, // all but the last, which is spelled in upper case (the DOS spelling)
  MISSING,   // a directory is missing, or the name is too long
};

struct Resolution {
  Found       found{ Found::MISSING };
  std::string path;
};

using UniqueAudioStream = std::unique_ptr<SDL_AudioStream, Destroy<SDL_DestroyAudioStream>>;

// The SDL window, renderer, audio stream and the last frame, behind platform.hpp.
class Backend {
public:
  auto               Init(PlatConfig const& config) -> int;
  auto               Shutdown() -> void;
  auto               Present(Frame const& frame) -> void;
  [[nodiscard]] auto WritePng(std::string const& path) const -> bool;
  [[nodiscard]] auto PollEvent() -> std::optional<InputEvent>;
  [[nodiscard]] auto TicksUs() const -> std::uint64_t;
  [[nodiscard]] auto Headless() const -> bool {
    return _headless;
  }
  auto               SetSaveDir(std::string_view dir) -> void;
  [[nodiscard]] auto Resolve(std::string_view dos_name) -> Resolution;
  [[nodiscard]] auto OpenAudio(int rate, AudioSource source) -> bool;
  auto               LockAudio() const -> void;
  auto               UnlockAudio() const -> void;

private:
  auto                OpenWindow(PlatConfig const& config, int width, int height, int scale) -> int;
  auto                Redraw() const -> void;
  auto                ReplaceTexture(int width, int height) -> void;
  auto                Convert(Frame const& frame) -> void;
  [[nodiscard]] auto  Translated(SDL_Event& sdl) const -> std::optional<InputEvent>;
  [[nodiscard]] auto  KeyEvent(SDL_KeyboardEvent const& key) const -> std::optional<InputEvent>;
  [[nodiscard]] auto  ButtonEvent(SDL_Event& sdl) const -> std::optional<InputEvent>;
  auto                Place(float fx, float fy, InputEvent& event) const -> void;
  [[nodiscard]] auto  SaveDir() -> std::string const&;
  static auto SDLCALL FillAudio(void* backend, SDL_AudioStream* stream, int additional, int total) -> void;

  bool          _inited{ false };
  bool          _headless{ false };
  SDL_InitFlags _sdl_flags{ 0 };
  std::uint64_t _t0_ns{ 0 };

  UniqueWindow   _window;
  UniqueRenderer _renderer;
  UniqueTexture  _texture;
  int            _texture_width{ 0 };
  int            _texture_height{ 0 };

  std::vector<std::uint8_t>               _frame; // the last indexed frame
  int                                     _frame_width{ 0 };
  int                                     _frame_height{ 0 };
  std::array<std::uint8_t, PALETTE_BYTES> _palette{};
  std::vector<std::uint32_t>              _converted; // the frame through the palette

  UniqueAudioStream _audio;
  AudioSource       _audio_source;

  std::string _save_dir;
};

// The one backend of the process; SDL itself is process-wide. The game-facing services reach it through the free
// functions of platform.hpp until the machine interface (M2 B5) hands it down.
auto TheBackend() -> Backend& {
  static Backend backend;
  return backend;
}

// ---- lifecycle ----------------------------------------------------------------------------------------------------

auto Backend::Init(PlatConfig const& config) -> int {
  if (_inited) return 0;
  _headless = config.headless != 0;
  auto const width{ config.width > 0 ? config.width : DEFAULT_WIDTH };
  auto const height{ config.height > 0 ? config.height : DEFAULT_HEIGHT };
  auto const scale{ std::max(config.scale, 1) };

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

auto Backend::OpenWindow(PlatConfig const& config, int const width, int const height, int const scale) -> int {
  auto const title{ std::string{ config.title.empty() ? DEFAULT_TITLE : config.title } };
  SDL_SetAppMetadata(title.c_str(), nullptr, nullptr);
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
  SDL_Window*   window{ nullptr };
  SDL_Renderer* renderer{ nullptr };
  auto const    created{ SDL_CreateWindowAndRenderer(title.c_str(), width * scale, height * scale, window_flags, &window, &renderer) };
  _window.reset(window);
  _renderer.reset(renderer);
  if (!created) {
    std::println(stderr, "plat_init: SDL_CreateWindowAndRenderer: {}", SDL_GetError());
    _renderer.reset();
    _window.reset();
    SDL_Quit();
    return -1;
  }
#ifndef __EMSCRIPTEN__
  SDL_SetRenderVSync(_renderer.get(), 1);
#endif
  SDL_SetRenderLogicalPresentation(_renderer.get(), width, height, PRESENTATION);
  if constexpr (HIDE_CURSOR) SDL_HideCursor();
  // Never deliver text-input events; the backend derives ASCII itself.
  SDL_StopTextInput(_window.get());
  SDL_SetRenderDrawColor(_renderer.get(), 0, 0, 0, 255);
  SDL_RenderClear(_renderer.get());
  SDL_RenderPresent(_renderer.get());
  return 0;
}

auto Backend::Shutdown() -> void {
  if (!_inited) return;
  _audio.reset();
  _texture.reset();
  _renderer.reset();
  _window.reset();
  _frame.clear();
  SDL_Quit();
  _inited = false;
}

// ---- video --------------------------------------------------------------------------------------------------------

// Show the last converted frame again: after each present, and after a window resize or exposure, when the window's
// contents are gone (a browser blanks a canvas whose size changed) and the game, waiting for input, may not draw again
// for a long time.
auto Backend::Redraw() const -> void {
  if (!_renderer || !_texture) return;
  SDL_SetRenderDrawColor(_renderer.get(), 0, 0, 0, 255);
  SDL_RenderClear(_renderer.get());
  SDL_RenderTexture(_renderer.get(), _texture.get(), nullptr, nullptr);
  SDL_RenderPresent(_renderer.get());
}

auto Backend::ReplaceTexture(int const width, int const height) -> void {
  _texture.reset(SDL_CreateTexture(_renderer.get(), SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, width, height));
  if (!_texture) return;
  SDL_SetTextureScaleMode(_texture.get(), SDL_SCALEMODE_NEAREST);
  _texture_width  = width;
  _texture_height = height;
  SDL_SetRenderLogicalPresentation(_renderer.get(), width, height, PRESENTATION);
}

// The frame through the palette into the texture.
auto Backend::Convert(Frame const& frame) -> void {
  std::array<std::uint32_t, 256> lut{};
  std::ranges::transform(std::views::iota(0uz, lut.size()), lut.begin(), [&frame](std::size_t const index) -> std::uint32_t {
    std::array<std::uint32_t, 3> rgb{};
    std::ranges::copy(frame.palette.subspan(index * 3, 3), rgb.begin());
    auto const [red, green, blue]{ rgb };
    return 0xFF000000u | red << 16 | green << 8 | blue;
  });
  _converted.resize(frame.pixels.size());
  std::ranges::transform(frame.pixels, _converted.begin(), [&lut](std::uint8_t const index) -> std::uint32_t { return lut.at(index); });
  SDL_UpdateTexture(_texture.get(), nullptr, _converted.data(), frame.width * static_cast<int>(sizeof(std::uint32_t)));
}

auto Backend::Present(Frame const& frame) -> void {
  _frame.assign(frame.pixels.begin(), frame.pixels.end());
  _frame_width  = frame.width;
  _frame_height = frame.height;
  std::ranges::copy(frame.palette, _palette.begin());

  if (!_renderer) return;
  if (!_texture || _texture_width != frame.width || _texture_height != frame.height) ReplaceTexture(frame.width, frame.height);
  if (!_texture) return;
  Convert(frame);
  Redraw();
#ifdef __EMSCRIPTEN__
  // Return to the browser so it shows the frame and queues input.
  emscripten_sleep(0);
#endif
}

auto Backend::WritePng(std::string const& path) const -> bool {
  if (_frame.empty() || path.empty()) return false;
  auto const rgb{ _frame | std::views::transform([this](std::uint8_t const index) { return std::span{ _palette }.subspan(static_cast<std::size_t>(index) * 3, 3); }) | std::views::join
                  | std::ranges::to<std::vector<std::uint8_t>>() };
  // 8-bit truecolour, filter type 0 on every row, so the image readers of the integration suites need no unfiltering.
  stbi_write_force_png_filter = 0;
  return stbi_write_png(path.c_str(), _frame_width, _frame_height, 3, rgb.data(), _frame_width * 3) != 0;
}

// ---- input --------------------------------------------------------------------------------------------------------

auto Backend::Place(float const fx, float const fy, InputEvent& event) const -> void {
  auto const extent{ [](int const texture, int const frame, int const fallback) -> int {
    if (texture > 0) return texture;
    return frame > 0 ? frame : fallback;
  } };
  auto const width{ extent(_texture_width, _frame_width, DEFAULT_WIDTH) };
  auto const height{ extent(_texture_height, _frame_height, DEFAULT_HEIGHT) };
  event.x = std::clamp(static_cast<int>(SDL_floorf(fx)), 0, width - 1);
  event.y = std::clamp(static_cast<int>(SDL_floorf(fy)), 0, height - 1);
}

// SDL key repeat arrives as further KEY_DOWN events (repeat=1), which is exactly the DOS typematic behaviour.
auto Backend::KeyEvent(SDL_KeyboardEvent const& key) const -> std::optional<InputEvent> {
  auto const found{ FindKey(key.scancode) };
  if (!found) return std::nullopt;
  auto const down{ key.type == SDL_EVENT_KEY_DOWN };
  auto       event{ Event(down ? EventType::KEY_DOWN : EventType::KEY_UP) };
  event.scancode = found->code;
  event.extended = found->extended;
  event.ascii    = down ? KeyAscii(*found, key.mod) : std::uint8_t{ 0 };
  return event;
}

auto Backend::ButtonEvent(SDL_Event& sdl) const -> std::optional<InputEvent> {
  auto const button{ ButtonNumber(sdl.button.button) };
  if (!button) return std::nullopt;
  SDL_ConvertEventToRenderCoordinates(_renderer.get(), &sdl);
  auto event{ Event(sdl.type == SDL_EVENT_MOUSE_BUTTON_DOWN ? EventType::MOUSE_DOWN : EventType::MOUSE_UP) };
  event.button = *button;
  Place(sdl.button.x, sdl.button.y, event);
  return event;
}

// The port's event for one of SDL's, if it is one the port uses.
auto Backend::Translated(SDL_Event& sdl) const -> std::optional<InputEvent> {
  switch (sdl.type) {
  case SDL_EVENT_QUIT:                      return Event(EventType::QUIT);
  case SDL_EVENT_WINDOW_RESIZED:
  case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
  case SDL_EVENT_WINDOW_EXPOSED:            Redraw(); return std::nullopt;
  case SDL_EVENT_KEY_DOWN:
  case SDL_EVENT_KEY_UP:                    return KeyEvent(sdl.key);
  case SDL_EVENT_MOUSE_MOTION:              {
    SDL_ConvertEventToRenderCoordinates(_renderer.get(), &sdl);
    auto event{ Event(EventType::MOUSE_MOVE) };
    Place(sdl.motion.x, sdl.motion.y, event);
    return event;
  }
  case SDL_EVENT_MOUSE_BUTTON_DOWN:
  case SDL_EVENT_MOUSE_BUTTON_UP:   return ButtonEvent(sdl);
  default:                          return std::nullopt;
  }
}

auto Backend::PollEvent() -> std::optional<InputEvent> {
  if (!_inited || _headless) return std::nullopt;
  SDL_Event sdl{};
  while (SDL_PollEvent(&sdl)) {
    if (auto const event{ Translated(sdl) }) return event;
  }
  return std::nullopt;
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
    auto const pref{ std::unique_ptr<char, Destroy<SDL_free>>{ SDL_GetPrefPath("opeu", "Operation Europe") } };
    SetSaveDir(pref ? std::string_view{ pref.get() } : std::string_view{ "." });
#endif
  }
  return _save_dir;
}

// Resolves a DOS name (with '\' or '/' separators) inside the save directory, matching every component
// case-insensitively; a missing last component is appended in upper case, as creatable.
auto Backend::Resolve(std::string_view const dos_name) -> Resolution {
  if (dos_name.empty()) return {};
  auto path{ SaveDir() };
  auto rest{ dos_name };
  while (!rest.empty()) {
    rest.remove_prefix(std::min(rest.find_first_not_of("\\/"), rest.size()));
    if (rest.empty()) break;
    auto const length{ static_cast<std::size_t>(std::ranges::find_if(rest, IsSeparator) - rest.begin()) };
    auto const component{ std::string{ rest.substr(0, length) } };
    if (length >= COMPONENT_MAX_LENGTH) return {};
    rest.remove_prefix(length);
    auto const last{ rest.find_first_not_of("\\/") == std::string_view::npos };
    if (path.size() + 1 + length + 1 > PATH_MAX_LENGTH) return {};
    // Fast path: the exact spelling exists.
    if (auto const exact{ path + "/" + component }; PathExists(exact)) {
      path = exact;
      continue;
    }
    if (auto const spelled{ Spelled(path, component) }) {
      if (path.size() + 1 + spelled->size() + 1 > PATH_MAX_LENGTH) return {};
      path += "/" + *spelled;
      continue;
    }
    if (!last) return {};
    path += '/';
    std::ranges::transform(component, std::back_inserter(path), Uppercase);
    return { .found = Found::CREATABLE, .path = path };
  }
  return { .found = Found::EXISTS, .path = path };
}

// ---- audio --------------------------------------------------------------------------------------------------------

auto SDLCALL Backend::FillAudio(void* const backend, SDL_AudioStream* const stream, int additional, [[maybe_unused]] int const total) -> void {
  auto const&                                                                self{ *static_cast<Backend const*>(backend) };
  std::array<std::int16_t, static_cast<std::size_t>(AUDIO_CHUNK_FRAMES) * 2> samples{};
  // `additional` is in bytes; one frame is 4 bytes (S16 stereo).
  while (additional >= 4) {
    auto const frames{ std::min(additional / 4, AUDIO_CHUNK_FRAMES) };
    auto const chunk{ std::span{ samples }.first(static_cast<std::size_t>(frames) * 2) };
    if (self._audio_source)
      self._audio_source(chunk);
    else
      std::ranges::fill(chunk, std::int16_t{ 0 });
    SDL_PutAudioStreamData(stream, chunk.data(), frames * 4);
    additional -= frames * 4;
  }
}

auto Backend::OpenAudio(int rate, AudioSource source) -> bool {
  if (!_inited || _headless) return false;
  if ((_sdl_flags & SDL_INIT_AUDIO) == 0) return false;
  if (rate <= 0) rate = DEFAULT_AUDIO_RATE;
  _audio.reset();
  _audio_source = std::move(source);
  SDL_AudioSpec const spec{ .format = SDL_AUDIO_S16, .channels = 2, .freq = rate };
  _audio.reset(SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, FillAudio, this));
  if (!_audio) {
    std::println(stderr, "plat_audio_open: {}", SDL_GetError());
    return false;
  }
  SDL_ResumeAudioStreamDevice(_audio.get());
  return true;
}

auto Backend::LockAudio() const -> void {
  if (_audio) SDL_LockAudioStream(_audio.get());
}

auto Backend::UnlockAudio() const -> void {
  if (_audio) SDL_UnlockAudioStream(_audio.get());
}

[[nodiscard]] auto OpenMode(FileMode const mode) -> std::ios::openmode {
  switch (mode) {
  case FileMode::READ:       return std::ios::in | std::ios::binary;
  case FileMode::READ_WRITE: return std::ios::in | std::ios::out | std::ios::binary;
  default:                   return std::ios::in | std::ios::out | std::ios::trunc | std::ios::binary;
  }
}

[[nodiscard]] auto Direction(SeekOrigin const origin) -> std::ios::seekdir {
  switch (origin) {
  case SeekOrigin::START:   return std::ios::beg;
  case SeekOrigin::CURRENT: return std::ios::cur;
  default:                  return std::ios::end;
  }
}

} // namespace
} // namespace opeu::platform::detail::sdl3

// ---- the platform contract ----------------------------------------------------------------------------------------

using opeu::platform::detail::sdl3::TheBackend;

auto plat_init(PlatConfig const* const cfg) -> int {
  return cfg == nullptr ? -1 : TheBackend().Init(*cfg);
}

auto plat_shutdown() -> void {
  TheBackend().Shutdown();
}

auto plat_set_save_dir(std::string_view const dir) -> void {
  TheBackend().SetSaveDir(dir);
}

namespace opeu::platform::detail::platform {

auto Present(Frame const& frame) -> void {
  if (frame.width <= 0 || frame.height <= 0) return;
  auto const count{ static_cast<std::size_t>(frame.width) * static_cast<std::size_t>(frame.height) };
  if (frame.pixels.size() < count) return;
  TheBackend().Present(Frame{ .pixels = frame.pixels.first(count), .width = frame.width, .height = frame.height, .palette = frame.palette });
}

auto WritePng(std::string const& path) -> bool {
  return TheBackend().WritePng(path);
}

auto PollEvent() -> std::optional<InputEvent> {
  return TheBackend().PollEvent();
}

auto TicksUs() -> std::uint64_t {
  return TheBackend().TicksUs();
}

auto SleepMs(int const ms) -> void {
#ifdef __EMSCRIPTEN__
  // Yield to the browser event loop (ASYNCIFY/JSPI unwinds the stack and resumes here from a timer).
  if (ms >= 0 && !TheBackend().Headless()) emscripten_sleep(static_cast<unsigned>(ms));
#else
  if (ms > 0) SDL_Delay(static_cast<Uint32>(ms));
#endif
}

SaveFile::SaveFile(std::fstream stream) : _stream{ std::move(stream) } { }

auto SaveFile::Open(std::string_view const dos_name, FileMode const mode) -> std::optional<SaveFile> {
  auto const [found, path]{ TheBackend().Resolve(dos_name) };
  auto const usable{ mode == FileMode::CREATE ? found != sdl3::Found::MISSING : found == sdl3::Found::EXISTS };
  if (!usable) return std::nullopt;
  std::fstream stream{ path, sdl3::OpenMode(mode) };
  if (!stream.is_open()) return std::nullopt;
  return SaveFile{ std::move(stream) };
}

auto SaveFile::Read(std::span<std::uint8_t> const bytes) -> std::size_t {
  // The stream reads chars; a save file is small, so it reads into chars and converts them to bytes.
  std::vector<char> chars(bytes.size());
  _stream.read(chars.data(), static_cast<std::streamsize>(chars.size()));
  auto const count{ static_cast<std::size_t>(_stream.gcount()) };
  _stream.clear();
  std::ranges::transform(chars | std::views::take(count), bytes.begin(), [](char const c) { return static_cast<std::uint8_t>(c); });
  return count;
}

auto SaveFile::Write(std::span<std::uint8_t const> const bytes) -> std::size_t {
  auto const chars{ bytes | std::views::transform([](std::uint8_t const b) { return static_cast<char>(b); }) | std::ranges::to<std::vector>() };
  _stream.write(chars.data(), static_cast<std::streamsize>(chars.size()));
  auto const written{ _stream.good() ? bytes.size() : 0uz };
  _stream.clear();
  return written;
}

auto SaveFile::Seek(std::int64_t const offset, SeekOrigin const origin) -> std::optional<std::int64_t> {
  _stream.clear();
  _stream.seekg(offset, sdl3::Direction(origin));
  if (_stream.fail()) {
    _stream.clear();
    return std::nullopt;
  }
  return static_cast<std::int64_t>(_stream.tellg());
}

auto SaveFile::Flush() -> void {
  _stream.flush();
  _stream.clear();
}

auto OpenAudio(int const rate, AudioSource source) -> bool {
  return TheBackend().OpenAudio(rate, std::move(source));
}

AudioLock::AudioLock() {
  TheBackend().LockAudio();
}

AudioLock::~AudioLock() {
  TheBackend().UnlockAudio();
}

}
