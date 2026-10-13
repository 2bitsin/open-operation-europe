#pragma once
// census: raii — remembers SDL's previous render target (a raw handle SDL owns) to put it back.
// A scope during which the renderer draws into a texture instead of the frame: composition through render targets.

#include "opeu/render/texture.hpp"

#include <SDL3/SDL.h>

#include <functional>

namespace opeu::render::detail::target {

class Target {
public:
  Target(SDL_Renderer& renderer, Texture& layer);
  ~Target();

  Target(Target const&)                    = delete;
  Target(Target&&)                         = delete;
  auto operator=(Target const&) -> Target& = delete;
  auto operator=(Target&&) -> Target&      = delete;

private:
  std::reference_wrapper<SDL_Renderer> _renderer;
  SDL_Texture*                         _previous; // the frame, another layer, or null for the window
};

}

namespace opeu::render {
using detail::target::Target;
}
