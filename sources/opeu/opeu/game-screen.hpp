// The screen the game draws on: services::Display on SDL_Renderer (render::RenderDisplay) over the game's pack (the
// checked-in classic pack compiled into the executable, a remaster laid over it when one is embedded:
// assets::OpenGamePack) and the content index that tells which of the game's byte images are shipped assets. The pump
// presents through it (services::ShownFrames, Display::Present).
#pragma once

#include "opeu/assets/content-index.hpp"
#include "opeu/assets/pack.hpp"
#include "opeu/assets/source.hpp"
#include "opeu/platform/sdl3/renderer-host.hpp"
#include "opeu/render/display.hpp"
#include "opeu/render/render-display.hpp"

#include <expected>
#include <memory>
#include <string>

namespace opeu::app::detail::game_screen {

// SDL's software renderer on a 640x480 surface and the frame on it. The window and the headless PNG take the frame as
// palette entries through the platform, so no window renderer is needed; the renderer draws the same pixels on every
// machine.
struct FrameSurface {
  platform::SurfaceRenderer renderer;
  render::Display           frame;
};

// What the screen draws from: the pack, and the shipped files the content index is built from.
struct ScreenAssets {
  std::unique_ptr<::opeu::assets::AssetPack const> pack;
  ::opeu::assets::TableSource                      files;
};

class GameScreen final {
  struct Key {
    explicit Key() = default;
  };

public:
  // The screen over the game's pack, recognising the images of the shipped files `files`; the reason when the pack
  // cannot be read or SDL cannot open the renderer.
  [[nodiscard]] static auto Open(::opeu::assets::TableSource files) -> std::expected<std::unique_ptr<GameScreen>, std::string>;

  // Open's: the display reads the pack and the index, which the screen owns.
  GameScreen(Key key, ScreenAssets assets, FrameSurface surface);
  GameScreen(GameScreen const&)                    = delete;
  GameScreen(GameScreen&&)                         = delete;
  auto operator=(GameScreen const&) -> GameScreen& = delete;
  auto operator=(GameScreen&&) -> GameScreen&      = delete;
  ~GameScreen()                                    = default;

  // What the game draws with.
  [[nodiscard]] auto Display() noexcept -> services::Display& {
    return _display;
  }

private:
  std::unique_ptr<::opeu::assets::AssetPack const> _pack;
  ::opeu::assets::ContentIndex                     _index;
  platform::SurfaceRenderer                        _renderer;
  render::RenderDisplay                            _display;
};

}

namespace opeu::app {
using detail::game_screen::GameScreen;
}
