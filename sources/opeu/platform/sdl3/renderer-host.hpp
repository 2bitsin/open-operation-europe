#pragma once
// census: raii — SDL's renderer API takes and gives raw handles; they stay inside these owners.
// Where an SDL_Renderer comes from: a window on the screen, or a surface in memory (SDL's software renderer, which
// draws the same frame on every machine: the tests' and the headless captures' renderer). The render module receives
// the renderer by reference from one of these and never creates one.

#include "opeu/platform/sdl3/handles.hpp"

#include <SDL3/SDL.h>

#include <cstdint>
#include <expected>
#include <string>

namespace opeu::platform::detail::renderer_host {

// Owns a renderer and lends it out.
class RendererHost {
public:
  virtual ~RendererHost() = default;

  [[nodiscard]] virtual auto Renderer() const -> SDL_Renderer& = 0;

protected:
  RendererHost()                                           = default;
  RendererHost(RendererHost const&)                        = default;
  RendererHost(RendererHost&&) noexcept                    = default;
  auto operator=(RendererHost const&) -> RendererHost&     = default;
  auto operator=(RendererHost&&) noexcept -> RendererHost& = default;
};

// SDL's video subsystem, held while a window lives (SDL counts the holders).
class VideoSubsystem {
public:
  [[nodiscard]] static auto Acquire() -> std::expected<VideoSubsystem, std::string>;

  VideoSubsystem(VideoSubsystem const&) = delete;
  VideoSubsystem(VideoSubsystem&& other) noexcept;
  auto operator=(VideoSubsystem const&) -> VideoSubsystem& = delete;
  auto operator=(VideoSubsystem&&) -> VideoSubsystem&      = delete;
  ~VideoSubsystem();

private:
  VideoSubsystem() = default;

  bool _held{ true };
};

struct WindowSpec {
  std::string title;
  int         width{};  // the window's size in pixels
  int         height{}; //
};

enum class WindowState : std::uint8_t {
  OPEN,
  CLOSE_REQUESTED,
};

// A resizable window and its renderer.
class WindowRenderer final : public RendererHost {
public:
  [[nodiscard]] static auto Open(WindowSpec const& spec) -> std::expected<WindowRenderer, std::string>;

  [[nodiscard]] auto Renderer() const -> SDL_Renderer& override;
  // Takes every pending event off the queue and says whether the user asked to close the window.
  static auto DrainEvents() -> WindowState;

private:
  WindowRenderer(VideoSubsystem video, UniqueWindow window, UniqueRenderer renderer);

  VideoSubsystem _video;
  UniqueWindow   _window;
  UniqueRenderer _renderer;
};

// SDL's software renderer drawing into a surface in memory: no window, no video subsystem, the same pixels on every
// machine.
class SurfaceRenderer final : public RendererHost {
public:
  [[nodiscard]] static auto Open(int width, int height) -> std::expected<SurfaceRenderer, std::string>;

  [[nodiscard]] auto Renderer() const -> SDL_Renderer& override;

private:
  SurfaceRenderer(UniqueSurface surface, UniqueRenderer renderer);

  UniqueSurface  _surface;
  UniqueRenderer _renderer;
};

}

namespace opeu::platform {
using detail::renderer_host::RendererHost;
using detail::renderer_host::SurfaceRenderer;
using detail::renderer_host::VideoSubsystem;
using detail::renderer_host::WindowRenderer;
using detail::renderer_host::WindowSpec;
using detail::renderer_host::WindowState;
}
