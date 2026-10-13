#include "opeu/render/game-frame.hpp"

#include "opeu/render/baking.hpp"
#include "opeu/render/colour-numbers.hpp"
#include "opeu/render/render-error.hpp"

#include <algorithm>
#include <ranges>
#include <utility>
#include <vector>

namespace opeu::render::detail::game_frame {
namespace {

// The part of `area` on the frame (empty when none is).
[[nodiscard]] auto OnFrame(Rect const area) -> Rect {
  auto const left{ std::max(area.x, 0) };
  auto const top{ std::max(area.y, 0) };
  auto const right{ std::min(area.x + area.width, LOGICAL_SIZE.width) };
  auto const bottom{ std::min(area.y + area.height, LOGICAL_SIZE.height) };
  return Rect{ .x = left, .y = top, .width = std::max(right - left, 0), .height = std::max(bottom - top, 0) };
}

// `inner`'s numbers at their place in `area`, 0 around them.
[[nodiscard]] auto Framed(assets::Image const& inner, Rect const placed, Rect const area) -> assets::Image {
  auto const width{ static_cast<std::size_t>(area.width) };
  auto       numbers{ std::vector<std::uint8_t>(width * static_cast<std::size_t>(area.height)) };
  auto const rows{ assets::Indices(inner.Height()) };
  std::ranges::for_each(rows, [&](std::size_t const row) {
    auto const source{ inner.Pixels().subspan(row * inner.Width(), inner.Width()) };
    auto const at{ (static_cast<std::size_t>(placed.y - area.y) + row) * width + static_cast<std::size_t>(placed.x - area.x) };
    std::ranges::copy(source, numbers.begin() + static_cast<std::ptrdiff_t>(at));
  });
  return assets::Image{ width, static_cast<std::size_t>(area.height), std::move(numbers) };
}

[[nodiscard]] auto SizeOf(assets::Image const& image) -> Size {
  return Size{ .width = static_cast<int>(image.Width()), .height = static_cast<int>(image.Height()) };
}

}

GameFrame::GameFrame(Display display, assets::PaletteColours const& colours) : _display{ std::move(display) }, _colours{ colours } {
  _display.Begin();
}

auto GameFrame::ColourOf(std::uint8_t const number) const -> Rgba {
  return Pen(_colours, number & 0x0FU);
}

auto GameFrame::Recolour(assets::PaletteColours const& colours) -> void {
  if (colours == _colours) return;
  auto const shown{ Required(_display.Read(LOGICAL_SCREEN)) };
  auto const texture{ Required(Texture::Upload(Renderer(), render::Recoloured(shown, _colours, colours))) };
  _display.Draw(texture, Blit{ .destination = LOGICAL_SCREEN });
  _colours = colours;
}

auto GameFrame::Restart(assets::PaletteColours const& colours) -> void {
  _colours = colours;
  Clear(0);
}

auto GameFrame::Clear(std::uint8_t const number) -> void {
  _display.Clear(ColourOf(number));
}

auto GameFrame::Fill(Rect const area, std::uint8_t const number) -> void {
  _display.Fill(area, ColourOf(number));
}

auto GameFrame::Plot(std::span<Point const> const points, std::uint8_t const number) -> void {
  _display.Plot(points, ColourOf(number));
}

auto GameFrame::Draw(Baked const& baked, Point const at) -> void {
  _display.Draw(baked.texture, Blit{ .destination = At(at, baked.size) });
}

auto GameFrame::Draw(Baked const& baked, Rect const destination) -> void {
  _display.Draw(baked.texture, Blit{ .destination = destination });
}

auto GameFrame::DrawInk(Baked const& shape, Rect const destination, std::uint8_t const number) -> void {
  _display.Draw(shape.texture, Blit{ .destination = destination, .modulation = ColourOf(number) });
}

auto GameFrame::Copy(Rect const source, Point const destination) -> void {
  Required(_display.Copy(source, destination));
}

auto GameFrame::Numbers(Rect const area) -> assets::Image {
  auto const placed{ OnFrame(area) };
  if (placed.width == 0 || placed.height == 0) return Framed(assets::Image{}, placed, area);
  auto const numbers{ NumbersOf(Required(_display.Read(placed)), _colours) };
  if (placed == area) return numbers;
  return Framed(numbers, placed, area);
}

auto GameFrame::Put(assets::Image const& numbers, Point const at) -> void {
  auto const texture{ Required(Texture::Upload(Renderer(), assets::ToTexture(numbers, _colours))) };
  _display.Draw(texture, Blit{ .destination = At(at, SizeOf(numbers)) });
}

auto GameFrame::Present(Rgba const tint) -> void {
  _display.Present(tint);
  _display.Resume();
}

auto GameFrame::Capture(Rgba const tint) -> assets::Texture {
  return Required(_display.Capture(tint));
}

}
