// census: raii — the previous render target is SDL's raw handle, held only to be restored.
#include "opeu/render/target.hpp"

namespace opeu::render::detail::target {

Target::~Target() {
  SDL_SetRenderTarget(&_renderer.get(), _previous);
}

Target::Target(SDL_Renderer& renderer, Texture& layer) : _renderer{ renderer }, _previous{ SDL_GetRenderTarget(&renderer) } {
  SDL_SetRenderTarget(&_renderer.get(), &layer.Handle());
}

}
