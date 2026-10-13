// What the port needs from the host machine: a window showing an 8-bit indexed frame through a 256-entry palette,
// input as PC set-1 scancodes with the BIOS ASCII and an absolute mouse in frame pixels, a monotonic clock, the files
// of the save directory and an audio device pulling stereo samples. The SDL3 backend (sdl3/platform-sdl3.cpp) is
// the one implementation, natively and in the browser. Every call works headless (no window, no audio device): a
// presented frame is only recorded for WritePng, and no event arrives.
#pragma once

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>

// ---- lifecycle: the entry's (opeu/entry.cpp, frozen until #17), so the names stay as the entry spells them ----

struct PlatConfig {
  std::string_view title;      // the window title
  int              width{};    // the frame's size (640x480)
  int              height{};   //
  int              scale{};    // the initial window scale (at least 1)
  int              headless{}; // 1: no window, no audio device, no input
  int              fullscreen{};
};

// NOLINTBEGIN(readability-identifier-naming): the entry calls these by their legacy names until #17
auto plat_init(PlatConfig const* cfg) -> int; // 0 on success, -1 on failure
auto plat_shutdown() -> void;
// The save directory: the only place the game's files touch the file system (the shipped data files are compiled
// into the executable and served by services/fileio.cpp). By default SDL's per-user pref path natively, /saves (the
// page's IDBFS mount) in the browser; created when missing.
auto plat_set_save_dir(std::string_view dir) -> void;
// NOLINTEND(readability-identifier-naming)

namespace opeu::platform::detail::platform {

inline constexpr auto PALETTE_BYTES{ 256uz * 3 };

// ---- video ----

// One frame: width x height palette indices, row-major, and 256 entries of 8-bit red, green, blue.
struct Frame {
  std::span<std::uint8_t const>                pixels;
  int                                          width{};
  int                                          height{};
  std::span<std::uint8_t const, PALETTE_BYTES> palette;
};

// Shows the frame (about 60 times a second at most) and keeps it for WritePng.
auto Present(Frame const& frame) -> void;
// Writes the last presented frame as a PNG file; false when there is none or the file cannot be written.
[[nodiscard]] auto WritePng(std::string const& path) -> bool;

// ---- input ----

enum class EventType : std::uint8_t {
  QUIT,       // window closed, Cmd-Q
  KEY_DOWN,   // scancode, extended, ascii
  KEY_UP,     // scancode, extended
  MOUSE_MOVE, // x, y (frame pixels, clamped)
  MOUSE_DOWN, // button (0 left, 1 right, 2 middle), x, y
  MOUSE_UP,   //
};

struct InputEvent {
  EventType    type{ EventType::QUIT };
  std::uint8_t scancode{}; // PC set-1 make code, bit 7 clear; 0xE0-prefixed keys (arrows, Home, ...) set extended
  std::uint8_t extended{};
  std::uint8_t ascii{}; // the BIOS INT 16h ASCII for this key with the current modifiers (0 for none)
  std::uint8_t button{};
  int          x{};
  int          y{};
};

// Pumps the host's event queue and takes the next event the port uses.
[[nodiscard]] auto PollEvent() -> std::optional<InputEvent>;

// ---- time ----

// Monotonic microseconds since plat_init.
[[nodiscard]] auto TicksUs() -> std::uint64_t;
// Yields to the host for `ms` milliseconds (in the browser: to its event loop).
auto SleepMs(int ms) -> void;

// ---- files of the save directory ----

enum class FileMode : std::uint8_t {
  READ,       // an existing file, read only
  READ_WRITE, // an existing file, read and written
  CREATE,     // a new or emptied file, read and written
};

enum class SeekOrigin : std::uint8_t {
  START,
  CURRENT,
  END,
};

// A file of the save directory, named as DOS names it ("SAVEDATA", '\' or '/' between directories) and found
// case-insensitively; a created file gets the upper-case spelling.
class SaveFile {
public:
  [[nodiscard]] static auto Open(std::string_view dos_name, FileMode mode) -> std::optional<SaveFile>;

  // Bytes transferred; fewer at the end of the file or on failure.
  [[nodiscard]] auto Read(std::span<std::uint8_t> bytes) -> std::size_t;
  [[nodiscard]] auto Write(std::span<std::uint8_t const> bytes) -> std::size_t;
  // The new position; nothing when it would precede the start.
  auto Seek(std::int64_t offset, SeekOrigin origin) -> std::optional<std::int64_t>;
  auto Flush() -> void;

private:
  explicit SaveFile(std::fstream stream);

  std::fstream _stream;
};

// ---- audio ----

// Fills interleaved stereo samples on the audio thread; it must never block.
using AudioSource = std::function<void(std::span<std::int16_t> stereo)>;

// Opens the audio device at `rate` Hz pulling from `source`; true when a device now pulls (never headless).
[[nodiscard]] auto OpenAudio(int rate, AudioSource source) -> bool;

// Holds the audio device's lock while it lives: the source does not run meanwhile.
class AudioLock {
public:
  AudioLock();
  AudioLock(AudioLock const&)                    = delete;
  AudioLock(AudioLock&&)                         = delete;
  auto operator=(AudioLock const&) -> AudioLock& = delete;
  auto operator=(AudioLock&&) -> AudioLock&      = delete;
  ~AudioLock();
};

}

namespace opeu::platform {
using detail::platform::AudioLock;
using detail::platform::AudioSource;
using detail::platform::EventType;
using detail::platform::FileMode;
using detail::platform::Frame;
using detail::platform::InputEvent;
using detail::platform::OpenAudio;
using detail::platform::PALETTE_BYTES;
using detail::platform::PollEvent;
using detail::platform::Present;
using detail::platform::SaveFile;
using detail::platform::SeekOrigin;
using detail::platform::SleepMs;
using detail::platform::TicksUs;
using detail::platform::WritePng;
}
