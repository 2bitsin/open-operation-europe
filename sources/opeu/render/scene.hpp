#pragma once
// One frame as the game describes it: logical operations in drawing order (a sprite by asset id, a filled
// rectangle, a line, a run of text) and the frame's tint (a fade, an hour's colour). A thin value: built per frame,
// drawn by SceneRenderer, then dropped; nothing is retained between frames.

#include "opeu/render/display.hpp"
#include "opeu/render/geometry.hpp"
#include "opeu/render/text-renderer.hpp"

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <variant>
#include <vector>

namespace opeu::render::detail::scene {

struct Sprite {
  std::string                asset{};   // an asset id, or an animation's name when `frame` is given
  std::optional<std::size_t> frame{};   //
  std::string                palette{}; // the palette to bake it in; empty: the image as the pack holds it
  Point                      at{};
  std::optional<Size>        size{}; // stretched to it; empty: the asset's own size
  Flip                       flip{ Flip::NONE };
  Rgba                       modulation{ WHITE };

  auto operator==(Sprite const&) const -> bool = default;
};

struct Fill {
  Rect area{};
  Rgba colour{};

  auto operator==(Fill const&) const -> bool = default;
};

struct Line {
  Point from{};
  Point to{};
  Rgba  colour{};

  auto operator==(Line const&) const -> bool = default;
};

using Text    = TextRun;
using Command = std::variant<Sprite, Fill, Line, Text>;

class Scene {
public:
  auto Add(Command command) -> Scene&;
  // Every pixel of the frame multiplied by `tint` when it is shown.
  auto Tint(Rgba tint) -> Scene&;

  [[nodiscard]] auto Commands() const noexcept -> std::span<Command const> {
    return _commands;
  }
  [[nodiscard]] auto Tinted() const noexcept -> Rgba {
    return _tint;
  }

  auto operator==(Scene const&) const -> bool = default;

private:
  std::vector<Command> _commands;
  Rgba                 _tint{ WHITE };
};

}

namespace opeu::render {
using detail::scene::Command;
using detail::scene::Fill;
using detail::scene::Line;
using detail::scene::Scene;
using detail::scene::Sprite;
using detail::scene::Text;
}
