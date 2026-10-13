// census: raii — main's argument vector is the operating system's; it becomes string views at once.
// Diagnostic: render-demo [--pack PATH] [--frames N] [--png PATH] shows the render module's demo scene (docs/rendering.md)
// from the classic pack in a window until it is closed or N frames have passed; with --png it draws N frames
// (default 1) on SDL's software renderer, no window, and writes the last one as presented.
#include "demo-scene.hpp"

#include "opeu/assets/pack.hpp"
#include "opeu/platform/sdl3/renderer-host.hpp"
#include "opeu/render/atlas-text-renderer.hpp"
#include "opeu/render/display.hpp"
#include "opeu/render/png-file.hpp"
#include "opeu/render/scene-renderer.hpp"
#include "opeu/render/texture-cache.hpp"

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <expected>
#include <filesystem>
#include <limits>
#include <optional>
#include <print>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace opeu::probes::render_demo {
namespace {

constexpr auto DEFAULT_PACK{ std::string_view{ "_local/packs/classic.opeupack" } };
constexpr auto CACHE_CAPACITY{ std::size_t{ 512 } };
constexpr auto WINDOW_SCALE{ 2 };

struct Options {
  std::filesystem::path                pack{ DEFAULT_PACK };
  std::optional<std::size_t>           frames;
  std::optional<std::filesystem::path> png;
};

// The word after `flag` on the command line.
[[nodiscard]] auto ValueOf(std::span<std::string_view const> const args, std::string_view const flag) -> std::optional<std::string_view> {
  auto const found{ std::ranges::find(args, flag) };
  if (found == args.end() || std::next(found) == args.end()) return std::nullopt;
  return *std::next(found);
}

[[nodiscard]] auto ParseCount(std::string_view const text) -> std::optional<std::size_t> {
  auto              count{ std::size_t{} };
  auto const* const end{ std::to_address(text.end()) };
  auto const        parsed{ std::from_chars(text.data(), end, count) };
  if (parsed.ec != std::errc{} || parsed.ptr != end || count == 0) return std::nullopt;
  return count;
}

[[nodiscard]] auto Parse(std::span<std::string_view const> const args) -> std::optional<Options> {
  auto options{ Options{} };
  if (auto const pack{ ValueOf(args, "--pack") }) options.pack = *pack;
  if (auto const png{ ValueOf(args, "--png") }) options.png = *png;
  if (auto const frames{ ValueOf(args, "--frames") }) {
    options.frames = ParseCount(*frames);
    if (!options.frames) return std::nullopt;
  }
  return options;
}

// Everything a frame is drawn with, on one renderer. Building it throws the first failure (an asset or font the pack
// cannot give, a texture SDL refuses) as std::bad_expected_access<RenderError>.
class Stage {
public:
  Stage(SDL_Renderer& renderer, assets::Pack const& pack)
      : _display{ render::Display::Open(renderer).value() }, _source{ pack }, _textures{ renderer, _source, CACHE_CAPACITY },
        _text{ render::AtlasTextRenderer::Build(renderer, _source, render::CLASSIC_FONTS).value() }, _day{ _source.Palette("main/day").value() } { }

  auto Show(std::size_t const frame) -> render::Rendered<render::Scene> {
    auto       scene{ DemoScene(frame, _day) };
    auto const shown{
      render::SceneRenderer{ _display, _textures, _text }
      .Show(scene)
    };
    if (!shown) return std::unexpected{ shown.error() };
    return scene;
  }
  auto Capture(render::Scene const& scene) -> render::Rendered<assets::Texture> {
    return _display.Capture(scene.Tinted());
  }

private:
  render::Display           _display;
  render::PackSource        _source;
  render::TextureCache      _textures;
  render::AtlasTextRenderer _text;
  assets::PaletteColours    _day;
};

auto Headless(assets::Pack const& pack, Options const& options) -> render::Rendered<void> {
  auto const host{ platform::SurfaceRenderer::Open(render::LOGICAL_SIZE.width, render::LOGICAL_SIZE.height) };
  if (!host) return render::Failed(host.error());
  auto stage{
    Stage{ host->Renderer(), pack }
  };
  auto last{ render::Rendered<render::Scene>{} };
  for (auto const frame : std::views::iota(0uz, options.frames.value_or(1))) {
    last = stage.Show(frame);
    if (!last) return std::unexpected{ last.error() };
  }
  auto const image{ stage.Capture(*last) };
  if (!image) return std::unexpected{ image.error() };
  return render::WritePng(*image, *options.png);
}

auto Windowed(assets::Pack const& pack, Options const& options) -> render::Rendered<void> {
  auto window{ platform::WindowRenderer::Open(
      platform::WindowSpec{ .title = "render demo", .width = render::LOGICAL_SIZE.width * WINDOW_SCALE, .height = render::LOGICAL_SIZE.height * WINDOW_SCALE }) };
  if (!window) return render::Failed(window.error());
  auto stage{
    Stage{ window->Renderer(), pack }
  };
  for (auto const frame : std::views::iota(0uz, options.frames.value_or(std::numeric_limits<std::size_t>::max()))) {
    if (window->DrainEvents() == platform::WindowState::CLOSE_REQUESTED) break;
    if (auto const shown{ stage.Show(frame) }; !shown) return std::unexpected{ shown.error() };
  }
  return {};
}

auto Run(std::span<std::string_view const> const args) -> int {
  auto const options{ Parse(args.subspan(1)) };
  if (!options) {
    std::println(stderr, "usage: render-demo [--pack PATH] [--frames N] [--png PATH]");
    return 2;
  }
  auto const pack{ assets::OpenPack(options->pack) };
  if (!pack) {
    std::println(stderr, "{}: {} (python3 tools/assets/pack.py builds the classic pack)", options->pack.string(), assets::Describe(pack.error()));
    return 1;
  }
  try {
    auto const done{ options->png ? Headless(*pack, *options) : Windowed(*pack, *options) };
    if (done) return 0;
    std::println(stderr, "{}", done.error().detail);
  } catch (std::bad_expected_access<render::RenderError> const& failure) {
    std::println(stderr, "{}", failure.error().detail);
  }
  return 1;
}

}
}

auto main(int const argc, char const* const* const argv) -> int {
  auto const args{
    std::span{ argv, static_cast<std::size_t>(argc) }
    | std::views::transform([](char const* const arg) { return std::string_view{ arg }; }
    ) | std::ranges::to<std::vector>()
  };
  return opeu::probes::render_demo::Run(args);
}
