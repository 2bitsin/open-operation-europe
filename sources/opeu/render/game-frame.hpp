#pragma once
// The frame the game draws on, in the game's terms: steps in colour numbers (frame-steps.hpp), painted in the sixteen
// colours the screen shows as they are added and kept as the frame's recipe. What the game asks of the pixels already
// drawn is answered from the recipe, never from the frame: a palette change paints the recipe again in the new colours,
// an XOR, AND or OR with the screen paints what is under it again through the operation's colour lookup, a saved
// rectangle or a battle cell is the part of the recipe under it, drawn again where it is put back. Coordinates are
// frame pixels (render::Display's); the game's line 0 is frame line 40.

#include "opeu/assets/content-index.hpp"
#include "opeu/assets/manifest.hpp"
#include "opeu/assets/pack.hpp"
#include "opeu/assets/texture.hpp"
#include "opeu/render/byte-textures.hpp"
#include "opeu/render/display.hpp"
#include "opeu/render/frame-steps.hpp"
#include "opeu/render/geometry.hpp"
#include "opeu/render/number-map.hpp"
#include "opeu/render/texture-cache.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace opeu::render::detail::game_frame {

inline constexpr auto GAME_TOP{ 40 };

class GameFrame {
public:
  // The frame on `display`, black in `colours`; the images whose bytes `index` recognises come from `pack`, and both
  // outlive the frame.
  GameFrame(Display display, assets::AssetPack const& pack, assets::ContentIndex const& index, assets::PaletteColours const& colours);

  [[nodiscard]] auto Colours() const noexcept -> assets::PaletteColours const& {
    return _colours;
  }
  // The colours change: every pixel shows its number's new colour.
  auto Recolour(assets::PaletteColours const& colours) -> void;
  // The colours change and the frame is colour 0.
  auto Restart(assets::PaletteColours const& colours) -> void;

  // A step drawn at the end of the recipe.
  auto Add(Step step) -> void;
  // The steps of `source` over `from` drawn again with that corner at `to`: a copy, a saved rectangle or a cell put back.
  auto AddCopy(Recipe const& source, Rect from, Point to) -> void;
  // What the frame shows over `area` (frame pixels), to be drawn again with AddCopy.
  [[nodiscard]] auto Under(Rect area) const -> Recipe {
    return _recipe.Touching(area);
  }
  [[nodiscard]] auto Steps() const noexcept -> Recipe const& {
    return _recipe;
  }
  // The overlay step named `overlay` taken out of the recipe, and the pixels it covered painted again.
  auto Withdraw(std::uint32_t overlay) -> void;
  // An identical XOR taken out of the recipe (a second XOR undoing the first), when nothing but XORs was drawn over it
  // since; whether it was.
  [[nodiscard]] auto WithdrawLast(RemapStep const& step) -> bool;

  // The frame to the renderer's output, every pixel multiplied by `tint`; drawing goes on into the frame.
  auto               Present(Rgba tint) -> void;
  [[nodiscard]] auto Capture(Rgba tint) -> assets::Texture;
  // Every pixel's colour number, the recipe painted in colours that tell the numbers apart and read back: the bridge to
  // the services' indexed present (render-display.hpp), not a step of the game's drawing. Painted and read only when
  // the frame changed since the last call; whether it did.
  [[nodiscard]] auto Numbers() -> assets::Image const&;
  [[nodiscard]] auto Changed() const noexcept -> bool {
    return _changed;
  }
  [[nodiscard]] auto Statistics() const noexcept -> ByteStatistics {
    return _textures.Statistics();
  }

private:
  // Where and how steps are painted: moved by `offset`, inside `clip` (target pixels), numbers through `map`.
  struct Lens {
    Point     offset;
    Rect      clip;
    NumberMap map{ SAME };
  };
  // What is under one shape painted again through a lookup.
  struct Remap {
    Rect                      area;
    std::optional<SharedBits> mask;
    NumberMap                 map{ SAME };
  };
  // An image's parts combined with what is under them, where the image lies.
  struct Combine {
    Rect                               area;
    std::span<CombineStep::Part const> parts;
  };
  // The two layers an XOR, AND or OR through a mask is composed in: what is under it, and the mask.
  struct Layers {
    Texture under;
    Texture shape;
  };

  auto               Repaint(Rect area) -> void;
  auto               Painted(Placed const& placed) -> void;
  auto               PaintSteps(std::span<Placed const> steps, Lens const& lens) -> void;
  auto               PaintStep(std::span<Placed const> before, Placed const& placed, Lens const& lens) -> void;
  auto               PaintFill(FillStep const& step, Lens const& lens) -> void;
  auto               PaintPoints(PointsStep const& step, Lens const& lens) -> void;
  auto               PaintInk(InkStep const& step, Lens const& lens) -> void;
  auto               PaintImage(ImageStep const& step, Lens const& lens) -> void;
  auto               PaintPicture(PictureStep const& step, Lens const& lens) -> void;
  auto               PaintRemap(std::span<Placed const> before, Remap const& remap, Lens const& lens) -> void;
  auto               PaintThroughMask(std::span<Placed const> before, Remap const& remap, Lens const& lens) -> void;
  auto               PaintCombine(std::span<Placed const> before, Combine const& combine, Lens const& lens) -> void;
  [[nodiscard]] auto ColourOf(std::uint8_t number, NumberMap const& map) const -> Rgba;
  [[nodiscard]] auto LayersAt(std::size_t depth) -> Layers&;

  Display                _display;
  TextureCache           _pack_textures;
  ByteTextures           _textures;
  assets::PaletteColours _colours;
  Recipe                 _recipe;
  std::vector<Layers>    _layers; // one pair per depth of masks within masks
  std::size_t            _depth{};
  assets::Image          _numbers;         // the last Numbers()
  bool                   _changed{ true }; // drawn on since Numbers() was last painted
  bool                   _numbering{};     // painting colour numbers: the game's own images, not the pack's
};

}

namespace opeu::render {
using detail::game_frame::GAME_TOP;
using detail::game_frame::GameFrame;
}
