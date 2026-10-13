#pragma once
// The frame the game draws on and the sixteen colours it shows, in the game's terms: drawing in colour numbers,
// reading colour numbers back, recolouring the pixels already drawn when the colours change. Coordinates are frame
// pixels (render::Display's); the game's line 0 is frame line 40.

#include "opeu/assets/image.hpp"
#include "opeu/assets/manifest.hpp"
#include "opeu/assets/texture.hpp"
#include "opeu/render/display.hpp"
#include "opeu/render/geometry.hpp"
#include "opeu/render/texture-cache.hpp"

#include <cstdint>
#include <span>

namespace opeu::render::detail::game_frame {

inline constexpr auto GAME_TOP{ 40 };

class GameFrame {
public:
  GameFrame(Display display, assets::PaletteColours const& colours);

  [[nodiscard]] auto Colours() const noexcept -> assets::PaletteColours const& {
    return _colours;
  }
  [[nodiscard]] auto ColourOf(std::uint8_t number) const -> Rgba;
  // The colours change: every pixel on the frame shows its number's new colour.
  auto Recolour(assets::PaletteColours const& colours) -> void;
  // The colours change and the frame is cleared to colour 0.
  auto Restart(assets::PaletteColours const& colours) -> void;

  auto Clear(std::uint8_t number) -> void;
  auto Fill(Rect area, std::uint8_t number) -> void;
  auto Plot(std::span<Point const> points, std::uint8_t number) -> void;
  // A texture at its game size from `at`, or stretched over `destination`.
  auto Draw(Baked const& baked, Point at) -> void;
  auto Draw(Baked const& baked, Rect destination) -> void;
  // A white shape in the colour of `number`.
  auto DrawInk(Baked const& shape, Rect destination, std::uint8_t number) -> void;
  auto Copy(Rect source, Point destination) -> void;

  // The colour numbers of `area` (0 outside the frame).
  [[nodiscard]] auto Numbers(Rect area) -> assets::Image;
  // Colour numbers drawn from `at`; the pixels the image masks out stay as they are.
  auto Put(assets::Image const& numbers, Point at) -> void;

  // The frame to the renderer's output, every pixel multiplied by `tint`; drawing goes on into the frame.
  auto               Present(Rgba tint) -> void;
  [[nodiscard]] auto Capture(Rgba tint) -> assets::Texture;
  [[nodiscard]] auto Renderer() const noexcept -> SDL_Renderer& {
    return _display.Renderer();
  }

private:
  Display                _display;
  assets::PaletteColours _colours;
};

}

namespace opeu::render {
using detail::game_frame::GAME_TOP;
using detail::game_frame::GameFrame;
}
