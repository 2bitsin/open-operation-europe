#include "game-screen.hpp"

#include "opeu/assets/catalogue.hpp"
#include "opeu/assets/embedded-pack.hpp"
#include "opeu/assets/pack-error.hpp"

#include <utility>

namespace opeu::app::detail::game_screen {
namespace {

constexpr auto SCREEN_WIDTH{ 640 };
constexpr auto SCREEN_HEIGHT{ 480 };

auto OpenAssets(::opeu::assets::TableSource files) -> std::expected<ScreenAssets, std::string> {
  auto pack{ ::opeu::assets::OpenGamePack() };
  if (!pack) return std::unexpected{ ::opeu::assets::Describe(pack.error()) };
  return ScreenAssets{ .pack = *std::move(pack), .files = std::move(files) };
}

auto OpenSurface() -> std::expected<FrameSurface, std::string> {
  auto renderer{ platform::SurfaceRenderer::Open(SCREEN_WIDTH, SCREEN_HEIGHT) };
  if (!renderer) return std::unexpected{ renderer.error() };
  auto frame{ render::Display::Open(renderer->Renderer()) }; // the renderer stays where it is when its owner moves
  if (!frame) return std::unexpected{ frame.error().detail };
  return FrameSurface{ .renderer = *std::move(renderer), .frame = *std::move(frame) };
}

}

auto GameScreen::Open(::opeu::assets::TableSource files) -> std::expected<std::unique_ptr<GameScreen>, std::string> {
  auto assets{ OpenAssets(std::move(files)) };
  if (!assets) return std::unexpected{ assets.error() };
  return OpenSurface().transform([&assets](FrameSurface surface) { return std::make_unique<GameScreen>(Key{}, *std::move(assets), std::move(surface)); });
}

GameScreen::GameScreen([[maybe_unused]] Key const key, ScreenAssets assets, FrameSurface surface)
    : _pack{ std::move(assets.pack) }, _index{ ::opeu::assets::Catalogue{ assets.files } }, _renderer{ std::move(surface.renderer) }, _display{ std::move(surface.frame), *_pack, _index } { }

}
