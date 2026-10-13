#pragma once
// The screen the game draws on: a 640x480 frame, composed with SDL_Renderer's 2D operations (blits with scaling,
// flips, alpha and colour modulation, fills, lines, points, render targets) and presented scaled to the window by
// SDL's logical presentation. The renderer is lent by the platform (a window, or a surface for headless frames).
//
//   display.Begin();                       // the frame is the target, cleared to black
//   display.Draw(texture, Blit{ ... });     // ... everything a frame shows, in order
//   display.Present(Dimmed(WHITE, level)); // the frame to the window, tinted: fades and hour tints
//
// docs/rendering.md is the design.

#include "opeu/assets/texture.hpp"
#include "opeu/render/geometry.hpp"
#include "opeu/render/render-error.hpp"
#include "opeu/render/target.hpp"
#include "opeu/render/texture.hpp"

#include <SDL3/SDL.h>

#include <cstdint>
#include <functional>
#include <optional>
#include <span>

namespace opeu::render::detail::display {

enum class Flip : std::uint8_t {
  NONE,
  HORIZONTAL,
  VERTICAL,
  BOTH,
};

// How a texture lands on the target.
struct Blit {
  Rect                destination{};       // scaled to fit when its size is not the source's
  std::optional<Rect> source{};            // the whole texture when empty
  Flip                flip{ Flip::NONE };  //
  Rgba                modulation{ WHITE }; // every pixel's colour and alpha are multiplied by it
};

// A rectangle of the target as it was, to be put back: what an XOR highlight drawn twice did.
struct Snapshot {
  Texture pixels;
  Rect    area;
};

class Display {
public:
  // The frame on `renderer`, whose output (the window, or a surface) shows it scaled.
  [[nodiscard]] static auto Open(SDL_Renderer& renderer) -> Rendered<Display>;

  // Starts a frame: the frame becomes the target, cleared to black.
  auto Begin() -> void;
  // Makes the frame the target again as it was (after a Present): a screen the game keeps drawing on.
  auto Resume() -> void;
  // Shows the frame, every pixel multiplied by `tint` (a fade, an hour's tint).
  auto Present(Rgba tint = WHITE) -> void;
  // The frame as Present shows it, at 640x480: the headless PNG capture.
  [[nodiscard]] auto Capture(Rgba tint = WHITE) -> Rendered<assets::Texture>;

  auto Clear(Rgba colour) -> void;
  auto Fill(Rect area, Rgba colour) -> void;
  auto Line(Point from, Point to, Rgba colour) -> void;
  auto Plot(Point at, Rgba colour) -> void;
  auto Plot(std::span<Point const> points, Rgba colour) -> void;
  auto Draw(Texture const& texture, Blit const& blit) -> void;
  // Carries out the drawing SDL has queued so far (Present, Read and a change of target do it anyway).
  auto Flush() -> void;

  // A transparent texture to compose into, and the scope during which drawing goes into it.
  [[nodiscard]] auto CreateLayer(Size size) const -> Rendered<Texture>;
  [[nodiscard]] auto Into(Texture& layer) const -> Target;

  [[nodiscard]] auto Take(Rect area) -> Rendered<Snapshot>;
  auto               Restore(Snapshot const& snapshot) -> void;
  // The target's `source` rectangle drawn again with its top left corner at `destination`, as it was before the copy
  // even where the two overlap: a scroll, a block moved or repeated across the screen.
  auto Copy(Rect source, Point destination) -> Rendered<void>;
  // The target's pixels in `area`, RGBA.
  [[nodiscard]] auto Read(Rect area) const -> Rendered<assets::Texture>;

  // The renderer the textures drawn here must come from.
  [[nodiscard]] auto Renderer() const noexcept -> SDL_Renderer& {
    return _renderer.get();
  }

private:
  Display(SDL_Renderer& renderer, Texture frame);

  auto ShowFrame(Rgba tint) -> void;
  auto SetColour(Rgba colour) -> void;

  std::reference_wrapper<SDL_Renderer> _renderer;
  Texture                              _frame;
};

}

namespace opeu::render {
using detail::display::Blit;
using detail::display::Display;
using detail::display::Flip;
using detail::display::Snapshot;
}
