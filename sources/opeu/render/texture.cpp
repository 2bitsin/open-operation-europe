#include "opeu/render/texture.hpp"

#include "opeu/render/rows.hpp"

#include <algorithm>
#include <cstddef>
#include <ranges>
#include <utility>

namespace opeu::render::detail::texture {
namespace {

[[nodiscard]] auto Created(SDL_Renderer& renderer, Size const size, SDL_TextureAccess const access) -> Rendered<platform::UniqueTexture> {
  auto handle{ platform::UniqueTexture{ SDL_CreateTexture(&renderer, SDL_PIXELFORMAT_RGBA32, access, size.width, size.height) } };
  if (!handle) return SdlFailed("SDL_CreateTexture");
  SDL_SetTextureBlendMode(handle.get(), SDL_BLENDMODE_BLEND);
  SDL_SetTextureScaleMode(handle.get(), SDL_SCALEMODE_NEAREST);
  return handle;
}

[[nodiscard]] auto OpacityOf(assets::Texture const& image) -> bool {
  auto const pixels{ Rows(image.Pixels(), RowLayout{ .pitch = 4, .width = 4, .count = image.Width() * image.Height() }) };
  return std::ranges::all_of(pixels, [](auto const pixel) { return pixel.back() == std::byte{ 0xFF }; });
}

[[nodiscard]] auto SizeOf(assets::Texture const& image) -> Size {
  return Size{ .width = static_cast<int>(image.Width()), .height = static_cast<int>(image.Height()) };
}

}

Texture::Texture(platform::UniqueTexture handle, Size const size, Opacity const opacity) : _handle{ std::move(handle) }, _size{ size }, _opaque{ opacity == Opacity::OPAQUE } { }

auto Texture::Upload(SDL_Renderer& renderer, assets::Texture const& image) -> Rendered<Texture> {
  auto const size{ SizeOf(image) };
  auto       handle{ Created(renderer, size, SDL_TEXTUREACCESS_STATIC) };
  if (!handle) return std::unexpected{ handle.error() };
  if (!SDL_UpdateTexture(handle->get(), nullptr, image.Pixels().data(), static_cast<int>(image.Pitch()))) return SdlFailed("SDL_UpdateTexture");
  return Texture{ *std::move(handle), size, OpacityOf(image) ? Opacity::OPAQUE : Opacity::TRANSLUCENT };
}

auto Texture::Target(SDL_Renderer& renderer, Size const size) -> Rendered<Texture> {
  auto handle{ Created(renderer, size, SDL_TEXTUREACCESS_TARGET) };
  if (!handle) return std::unexpected{ handle.error() };
  auto* const previous{ SDL_GetRenderTarget(&renderer) };
  SDL_SetRenderTarget(&renderer, handle->get());
  SDL_SetRenderDrawColor(&renderer, 0, 0, 0, 0);
  SDL_RenderClear(&renderer);
  SDL_SetRenderTarget(&renderer, previous);
  return Texture{ *std::move(handle), size, Opacity::TRANSLUCENT };
}

}
