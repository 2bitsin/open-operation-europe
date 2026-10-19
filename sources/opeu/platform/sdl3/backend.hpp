#pragma once
// census: raii — SDL's handles live in unique_ptr owners (handles.hpp); the raw pointers left are the audio callback's
// context and stream, as SDL's callback signature spells them.
// The SDL3 backend behind platform.hpp: SDL's window, renderer, audio stream, the last frame and the save directory,
// one object. platform-sdl3.cpp defines it; until the services own it (services::Host, docs/platform-surface.md) the
// free functions of platform.hpp reach the process's one instance.

#include "opeu/platform/platform.hpp"
#include "opeu/platform/sdl3/handles.hpp"

#include <SDL3/SDL.h>

#include <array>
#include <cstdint>
#include <expected>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace opeu::platform::detail::backend {

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

// The SDL window, renderer, audio stream, the last frame and the save directory: what platform.hpp's functions reach.
class Backend {
public:
  // Opens SDL (headless: timers only; otherwise the window, its renderer and, when there is a device, audio).
  [[nodiscard]] auto Init(PlatConfig const& config) -> std::expected<void, std::string>;
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
  [[nodiscard]] auto        OpenWindow(PlatConfig const& config) -> std::expected<void, std::string>;
  [[nodiscard]] auto        InitSubsystems() -> std::expected<void, std::string>;
  [[nodiscard]] auto        MakeWindow(PlatConfig const& config, std::string const& title) -> std::expected<void, std::string>;
  auto                      ClearWindow() const -> void;
  auto                      Redraw() const -> void;
  auto                      ReplaceTexture(int width, int height) -> void;
  auto                      Convert(Frame const& frame) -> void;
  [[nodiscard]] auto        Translated(SDL_Event& sdl) const -> std::optional<InputEvent>;
  [[nodiscard]] static auto KeyEvent(SDL_KeyboardEvent const& key) -> std::optional<InputEvent>;
  [[nodiscard]] auto        ButtonEvent(SDL_Event& sdl) const -> std::optional<InputEvent>;
  auto                      Place(float fx, float fy, InputEvent& event) const -> void;
  [[nodiscard]] auto        SaveDir() -> std::string const&;
  auto                      FillAudio(SDL_AudioStream& stream, int bytes) const -> void;

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

}

namespace opeu::platform {
using detail::backend::Backend;
using detail::backend::Found;
using detail::backend::Resolution;
}
