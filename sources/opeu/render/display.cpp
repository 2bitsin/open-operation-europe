#include "opeu/render/display.hpp"

#include "opeu/platform/sdl3/handles.hpp"
#include "opeu/render/rows.hpp"

#include <algorithm>
#include <cstddef>
#include <ranges>
#include <span>
#include <utility>
#include <vector>

namespace opeu::render::detail::display {
namespace {

// Whole multiples of the frame, letterboxed: every game pixel the same size on screen.
constexpr auto PRESENTATION{ SDL_LOGICAL_PRESENTATION_INTEGER_SCALE };

[[nodiscard]] auto ToSdl(Rect const rect) noexcept -> SDL_FRect {
  return SDL_FRect{ .x = static_cast<float>(rect.x), .y = static_cast<float>(rect.y), .w = static_cast<float>(rect.width), .h = static_cast<float>(rect.height) };
}

[[nodiscard]] auto ToSdlPixels(Rect const rect) noexcept -> SDL_Rect {
  return SDL_Rect{ .x = rect.x, .y = rect.y, .w = rect.width, .h = rect.height };
}

[[nodiscard]] auto ToSdl(Flip const flip) noexcept -> SDL_FlipMode {
  switch (flip) {
  case Flip::NONE:       return SDL_FLIP_NONE;
  case Flip::HORIZONTAL: return SDL_FLIP_HORIZONTAL;
  case Flip::VERTICAL:   return SDL_FLIP_VERTICAL;
  case Flip::BOTH:       return static_cast<SDL_FlipMode>(SDL_FLIP_HORIZONTAL | SDL_FLIP_VERTICAL);
  }
  return SDL_FLIP_NONE;
}

// The modulation, and copying instead of blending where the result is the same: an opaque texture drawn opaque.
auto Modulate(Texture const& texture, Rgba const modulation) -> void {
  SDL_SetTextureColorMod(&texture.Handle(), modulation.r, modulation.g, modulation.b);
  SDL_SetTextureAlphaMod(&texture.Handle(), modulation.a);
  SDL_SetTextureBlendMode(&texture.Handle(), texture.Opaque() && modulation.a == 0xFF ? SDL_BLENDMODE_NONE : SDL_BLENDMODE_BLEND);
}

// A surface's pixels, row by row without the padding SDL may add to each row.
[[nodiscard]] auto PixelsOf(SDL_Surface const& surface) -> std::vector<std::byte> {
  auto const height{ static_cast<std::size_t>(surface.h) };
  auto const layout{
    RowLayout{ .pitch = static_cast<std::size_t>(surface.pitch), .width = static_cast<std::size_t>(surface.w) * 4, .count = height }
  };
  auto const bytes{
    std::span{ static_cast<std::byte const*>(surface.pixels), layout.pitch * height }
  };
  if (layout.pitch == layout.width) return bytes | std::ranges::to<std::vector>();
  auto pixels{ std::vector<std::byte>{} };
  pixels.reserve(layout.width * height);
  std::ranges::for_each(Rows(bytes, layout), [&pixels](auto const row) { pixels.insert(pixels.end(), row.begin(), row.end()); });
  return pixels;
}

}

[[nodiscard]] auto ImageOf(SDL_Surface const& surface) -> assets::Texture {
  return assets::Texture{ static_cast<std::size_t>(surface.w), static_cast<std::size_t>(surface.h), PixelsOf(surface) };
}

Display::Display(SDL_Renderer& renderer, Texture frame) : _renderer{ renderer }, _frame{ std::move(frame) } { }

auto Display::Open(SDL_Renderer& renderer) -> Rendered<Display> {
  if (!SDL_SetRenderLogicalPresentation(&renderer, LOGICAL_SIZE.width, LOGICAL_SIZE.height, PRESENTATION)) return SdlFailed("SDL_SetRenderLogicalPresentation");
  auto frame{ Texture::Target(renderer, LOGICAL_SIZE) };
  if (!frame) return std::unexpected{ frame.error() };
  return Display{ renderer, *std::move(frame) };
}

auto Display::Begin() -> void {
  SDL_SetRenderTarget(&Renderer(), &_frame.Handle());
  Clear(BLACK);
}

auto Display::Present(Rgba const tint) -> void {
  SDL_SetRenderTarget(&Renderer(), nullptr);
  Clear(BLACK);
  ShowFrame(tint);
  SDL_RenderPresent(&Renderer());
}

auto Display::Capture(Rgba const tint) -> Rendered<assets::Texture> {
  auto shown{ Texture::Target(Renderer(), LOGICAL_SIZE) };
  if (!shown) return std::unexpected{ shown.error() };
  auto const into{ Into(*shown) };
  Clear(BLACK);
  ShowFrame(tint);
  return Read(LOGICAL_SCREEN);
}

auto Display::Clear(Rgba const colour) -> void {
  SetColour(colour);
  SDL_RenderClear(&Renderer());
}

auto Display::Fill(Rect const area, Rgba const colour) -> void {
  SetColour(colour);
  auto const rect{ ToSdl(area) };
  SDL_RenderFillRect(&Renderer(), &rect);
}

auto Display::Line(Point const from, Point const to, Rgba const colour) -> void {
  SetColour(colour);
  SDL_RenderLine(&Renderer(), static_cast<float>(from.x), static_cast<float>(from.y), static_cast<float>(to.x), static_cast<float>(to.y));
}

auto Display::Plot(Point const at, Rgba const colour) -> void {
  SetColour(colour);
  SDL_RenderPoint(&Renderer(), static_cast<float>(at.x), static_cast<float>(at.y));
}

auto Display::Draw(Texture const& texture, Blit const& blit) -> void {
  Modulate(texture, blit.modulation);
  auto const destination{ ToSdl(blit.destination) };
  auto const source{ ToSdl(blit.source.value_or(texture.Whole())) };
  SDL_RenderTextureRotated(&Renderer(), &texture.Handle(), &source, &destination, 0.0, nullptr, ToSdl(blit.flip));
}

auto Display::Flush() -> void {
  SDL_FlushRenderer(&Renderer());
}

auto Display::CreateLayer(Size const size) const -> Rendered<Texture> {
  return Texture::Target(Renderer(), size);
}

auto Display::Into(Texture& layer) const -> Target {
  return Target{ Renderer(), layer };
}

auto Display::Take(Rect const area) -> Rendered<Snapshot> {
  auto* const source{ SDL_GetRenderTarget(&Renderer()) };
  if (source == nullptr) return Failed("a snapshot is taken of the frame or a layer, not of the window");
  auto pixels{ Texture::Target(Renderer(), Dimensions(area)) };
  if (!pixels) return std::unexpected{ pixels.error() };
  auto blend{ SDL_BLENDMODE_NONE };
  SDL_GetTextureBlendMode(source, &blend);
  SDL_SetTextureBlendMode(source, SDL_BLENDMODE_NONE);
  {
    auto const into{ Into(*pixels) };
    auto const from{ ToSdl(area) };
    SDL_RenderTexture(&Renderer(), source, &from, nullptr);
  }
  SDL_SetTextureBlendMode(source, blend);
  SDL_SetTextureBlendMode(&pixels->Handle(), SDL_BLENDMODE_NONE); // a restore replaces, it does not blend
  return Snapshot{ .pixels = *std::move(pixels), .area = area };
}

auto Display::Restore(Snapshot const& snapshot) -> void {
  Draw(snapshot.pixels, Blit{ .destination = snapshot.area });
}

auto Display::Read(Rect const area) const -> Rendered<assets::Texture> {
  auto const rect{ ToSdlPixels(area) };
  auto const read{ platform::UniqueSurface{ SDL_RenderReadPixels(&Renderer(), &rect) } };
  if (!read) return SdlFailed("SDL_RenderReadPixels");
  if (read->format == SDL_PIXELFORMAT_RGBA32) return ImageOf(*read);
  auto const rgba{ platform::UniqueSurface{ SDL_ConvertSurface(read.get(), SDL_PIXELFORMAT_RGBA32) } };
  if (!rgba) return SdlFailed("SDL_ConvertSurface");
  return ImageOf(*rgba);
}

// The frame is opaque (Begin clears it opaque, and blending keeps it so): it is copied, tinted.
auto Display::ShowFrame(Rgba const tint) -> void {
  SDL_SetTextureColorMod(&_frame.Handle(), tint.r, tint.g, tint.b);
  SDL_SetTextureBlendMode(&_frame.Handle(), SDL_BLENDMODE_NONE);
  auto const screen{ ToSdl(LOGICAL_SCREEN) };
  SDL_RenderTexture(&Renderer(), &_frame.Handle(), nullptr, &screen);
}

// An opaque colour replaces what is there, which is what blending it gives too, only faster.
auto Display::SetColour(Rgba const colour) -> void {
  SDL_SetRenderDrawColor(&Renderer(), colour.r, colour.g, colour.b, colour.a);
  SDL_SetRenderDrawBlendMode(&Renderer(), colour.a == 0xFF ? SDL_BLENDMODE_NONE : SDL_BLENDMODE_BLEND);
}

}
