#pragma once
// census: raii — the owners of SDL's handles: every raw handle SDL gives out lives only inside a unique_ptr whose
// deleter is SDL's own destroy call, and the rest of the port sees references.

#include <SDL3/SDL.h>

#include <functional>
#include <memory>

namespace opeu::platform::detail::handles {

// A deleter calling one of SDL's destroy functions.
template <auto DESTROY>
struct Destroy {
  template <class _Handle>
  auto operator()(_Handle* const handle) const noexcept -> void {
    std::invoke(DESTROY, handle);
  }
};

using UniqueRenderer = std::unique_ptr<SDL_Renderer, Destroy<SDL_DestroyRenderer>>;
using UniqueSurface  = std::unique_ptr<SDL_Surface, Destroy<SDL_DestroySurface>>;
using UniqueTexture  = std::unique_ptr<SDL_Texture, Destroy<SDL_DestroyTexture>>;
using UniqueWindow   = std::unique_ptr<SDL_Window, Destroy<SDL_DestroyWindow>>;

}

namespace opeu::platform {
using detail::handles::Destroy;
using detail::handles::UniqueRenderer;
using detail::handles::UniqueSurface;
using detail::handles::UniqueTexture;
using detail::handles::UniqueWindow;
}
