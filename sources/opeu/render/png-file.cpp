#include "opeu/render/png-file.hpp"

#include "opeu/platform/sdl3/handles.hpp"
#include "opeu/render/rows.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstddef>
#include <ranges>
#include <span>

namespace opeu::render::detail::png_file {

auto WritePng(assets::Texture const& image, std::filesystem::path const& path) -> Rendered<void> {
  auto const surface{ platform::UniqueSurface{ SDL_CreateSurface(static_cast<int>(image.Width()), static_cast<int>(image.Height()), SDL_PIXELFORMAT_RGBA32) } };
  if (!surface) return SdlFailed("SDL_CreateSurface");
  auto const target{
    RowLayout{ .pitch = static_cast<std::size_t>(surface->pitch), .width = image.Pitch(), .count = image.Height() }
  };
  auto const source{
    RowLayout{ .pitch = image.Pitch(), .width = image.Pitch(), .count = image.Height() }
  };
  auto const pixels{
    std::span{ static_cast<std::byte*>(surface->pixels), target.pitch * target.count }
  };
  std::ranges::for_each(std::views::zip(Rows(pixels, target), Rows(image.Pixels(), source)), [](auto const rows) { std::ranges::copy(std::get<1>(rows), std::get<0>(rows).begin()); });
  if (!SDL_SavePNG(surface.get(), path.string().c_str())) return SdlFailed("SDL_SavePNG");
  return {};
}

}
