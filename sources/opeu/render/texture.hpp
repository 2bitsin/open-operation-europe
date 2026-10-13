#pragma once
// A picture in the renderer's memory: an SDL texture and its size. Uploaded from an RGBA image, or created as a render
// target the Display composes into.

#include "opeu/assets/texture.hpp"
#include "opeu/platform/sdl3/handles.hpp"
#include "opeu/render/geometry.hpp"
#include "opeu/render/render-error.hpp"

#include <SDL3/SDL.h>

#include <cstdint>

namespace opeu::render::detail::texture {

class Texture {
public:
  // The image as a static texture: alpha blended, scaled nearest-neighbour.
  [[nodiscard]] static auto Upload(SDL_Renderer& renderer, assets::Texture const& image) -> Rendered<Texture>;
  // A texture the renderer can draw into, cleared to transparent.
  [[nodiscard]] static auto Target(SDL_Renderer& renderer, Size size) -> Rendered<Texture>;

  [[nodiscard]] auto Dimensions() const noexcept -> Size {
    return _size;
  }
  [[nodiscard]] auto Whole() const noexcept -> Rect {
    return At(Point{}, _size);
  }
  // Whether every pixel is opaque: such a texture is copied, not blended, unless it is drawn translucent.
  [[nodiscard]] auto Opaque() const noexcept -> bool {
    return _opaque;
  }
  // SDL's handle, for the renderer's calls; the texture keeps owning it.
  [[nodiscard]] auto Handle() const noexcept -> SDL_Texture& {
    return *_handle;
  }

private:
  enum class Opacity : std::uint8_t {
    OPAQUE,
    TRANSLUCENT,
  };

  Texture(platform::UniqueTexture handle, Size size, Opacity opacity);

  platform::UniqueTexture _handle;
  Size                    _size;
  bool                    _opaque;
};

}

namespace opeu::render {
using detail::texture::Texture;
}
