#include "game-screen.hpp"

#include "embedded.hpp"

#include "opeu/assets/catalogue-pack.hpp"
#include "opeu/assets/catalogue.hpp"
#include "opeu/assets/layered-pack.hpp"
#include "opeu/assets/pack-error.hpp"

#include <utility>

namespace opeu::app::detail::game_screen {
namespace {

constexpr auto SCREEN_WIDTH{ 640 };
constexpr auto SCREEN_HEIGHT{ 480 };

// The pack the game draws without a remaster: the embedded classic pack, else the decoders' pack over `data`.
auto BasePack(::opeu::assets::GameData const& data) -> ::opeu::assets::Packed<std::unique_ptr<::opeu::assets::AssetPack const>> {
  auto embedded{ OpenEmbeddedPack(CLASSIC_PACK) };
  if (!embedded) return std::make_unique<::opeu::assets::CataloguePack const>(::opeu::assets::Catalogue{ data });
  if (!*embedded) return std::unexpected{ embedded->error() };
  return std::make_unique<::opeu::assets::Pack const>(**std::move(embedded));
}

// The base pack with the remaster at `remaster` over it, else the embedded remaster, if the build has one.
auto GamePack(::opeu::assets::GameData const& data, std::optional<std::filesystem::path> const& remaster) -> ::opeu::assets::Packed<std::unique_ptr<::opeu::assets::AssetPack const>> {
  auto base{ BasePack(data) };
  if (!base) return base;
  if (remaster) return ::opeu::assets::OpenLayered(*std::move(base), remaster);
  return ::opeu::assets::OpenLayered(*std::move(base), EmbeddedPackFiles(REMASTER_PACK));
}

auto OpenAssets(::opeu::assets::GameData const& data, std::optional<std::filesystem::path> const& remaster) -> std::expected<ScreenAssets, std::string> {
  auto pack{ GamePack(data, remaster) };
  if (!pack) return std::unexpected{ ::opeu::assets::Describe(pack.error()) };
  return ScreenAssets{ .pack = *std::move(pack), .data = data };
}

auto OpenSurface() -> std::expected<FrameSurface, std::string> {
  auto renderer{ platform::SurfaceRenderer::Open(SCREEN_WIDTH, SCREEN_HEIGHT) };
  if (!renderer) return std::unexpected{ renderer.error() };
  auto frame{ render::Display::Open(renderer->Renderer()) }; // the renderer stays where it is when its owner moves
  if (!frame) return std::unexpected{ frame.error().detail };
  return FrameSurface{ .renderer = *std::move(renderer), .frame = *std::move(frame) };
}

}

auto GameScreen::Open(::opeu::assets::GameData const& data, std::optional<std::filesystem::path> const& remaster) -> std::expected<std::unique_ptr<GameScreen>, std::string> {
  auto assets{ OpenAssets(data, remaster) };
  if (!assets) return std::unexpected{ assets.error() };
  return OpenSurface().transform([&assets](FrameSurface surface) { return std::make_unique<GameScreen>(Key{}, *std::move(assets), std::move(surface)); });
}

GameScreen::GameScreen([[maybe_unused]] Key const key, ScreenAssets assets, FrameSurface surface)
    : _pack{ std::move(assets.pack) }, _index{ ::opeu::assets::Catalogue{ assets.data.get() } }, _renderer{ std::move(surface.renderer) }, _display{ std::move(surface.frame), *_pack, _index } { }

}
