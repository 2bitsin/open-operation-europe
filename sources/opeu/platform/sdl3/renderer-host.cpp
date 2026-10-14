// census: raii — SDL's creation calls hand back raw handles, which go straight into their owners.
#include "opeu/platform/sdl3/renderer-host.hpp"

#include "opeu/platform/sdl3/target.hpp"

#include <format>
#include <string_view>
#include <utility>

namespace opeu::platform::detail::renderer_host {
namespace {

[[nodiscard]] auto Failure(std::string_view const call) -> std::unexpected<std::string> {
  return std::unexpected{ std::format("{}: {}", call, SDL_GetError()) };
}

[[nodiscard]] auto CreateWindow(WindowSpec const& spec) -> std::expected<UniqueWindow, std::string> {
  auto const flags{ SDL_WindowFlags{ SDL_WINDOW_RESIZABLE | target::ExtraWindowFlags() } };
  auto       window{ UniqueWindow{ SDL_CreateWindow(spec.title.c_str(), spec.width, spec.height, flags) } };
  if (!window) return Failure("SDL_CreateWindow");
  return window;
}

}

auto VideoSubsystem::Acquire() -> std::expected<VideoSubsystem, std::string> {
  if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) return Failure("SDL_InitSubSystem(video)");
  return VideoSubsystem{};
}

VideoSubsystem::~VideoSubsystem() {
  if (_held) SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

VideoSubsystem::VideoSubsystem(VideoSubsystem&& other) noexcept : _held{ std::exchange(other._held, false) } { }

auto WindowRenderer::Open(WindowSpec const& spec) -> std::expected<WindowRenderer, std::string> {
  auto video{ VideoSubsystem::Acquire() };
  if (!video) return std::unexpected{ video.error() };
  auto window{ CreateWindow(spec) };
  if (!window) return std::unexpected{ window.error() };
  auto renderer{ UniqueRenderer{ SDL_CreateRenderer(window->get(), nullptr) } };
  if (!renderer) return Failure("SDL_CreateRenderer");
  target::PaceFrames(*renderer);
  return WindowRenderer{ *std::move(video), *std::move(window), std::move(renderer) };
}

WindowRenderer::WindowRenderer(VideoSubsystem video, UniqueWindow window, UniqueRenderer renderer) : _video{ std::move(video) }, _window{ std::move(window) }, _renderer{ std::move(renderer) } { }

auto WindowRenderer::Renderer() const -> SDL_Renderer& {
  return *_renderer;
}

auto WindowRenderer::DrainEvents() -> WindowState {
  SDL_PumpEvents();
  auto const quit{ SDL_HasEvent(SDL_EVENT_QUIT) };
  SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);
  return quit ? WindowState::CLOSE_REQUESTED : WindowState::OPEN;
}

auto SurfaceRenderer::Open(int const width, int const height) -> std::expected<SurfaceRenderer, std::string> {
  auto surface{ UniqueSurface{ SDL_CreateSurface(width, height, SDL_PIXELFORMAT_RGBA32) } };
  if (!surface) return Failure("SDL_CreateSurface");
  auto renderer{ UniqueRenderer{ SDL_CreateSoftwareRenderer(surface.get()) } };
  if (!renderer) return Failure("SDL_CreateSoftwareRenderer");
  return SurfaceRenderer{ std::move(surface), std::move(renderer) };
}

SurfaceRenderer::SurfaceRenderer(UniqueSurface surface, UniqueRenderer renderer) : _surface{ std::move(surface) }, _renderer{ std::move(renderer) } { }

auto SurfaceRenderer::Renderer() const -> SDL_Renderer& {
  return *_renderer;
}

}
