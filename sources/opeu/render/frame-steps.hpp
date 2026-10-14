#pragma once
// The frame as a recipe: the steps that drew what it shows, in order, each in the game's colour numbers. The frame's
// pixels are the recipe painted in the live colours; painting it again through a colour lookup (number-map.hpp) is how
// the renderer shows a palette change, an XOR, AND or OR with the screen, a saved rectangle put back and a battle cell
// composed over the screen, all without reading the frame back. A copy is the steps under its source placed again,
// moved and clipped to where it lands, so no step holds another recipe. A step that covers others whole lets them go.

#include "opeu/assets/catalogue.hpp"
#include "opeu/assets/picture-stream.hpp"
#include "opeu/assets/planar.hpp"
#include "opeu/render/baking.hpp"
#include "opeu/render/byte-textures.hpp"
#include "opeu/render/geometry.hpp"
#include "opeu/render/number-map.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <variant>
#include <vector>

namespace opeu::render::detail::frame_steps {

// A one-bit image: rows of whole bytes, the leftmost pixel in bit 7, a 1 bit part of the shape; with the catalogue
// layout of the shipped asset its bytes may be.
class Bits {
public:
  Bits(std::vector<std::byte> bytes, Size size, std::optional<assets::Layout> shipped = std::nullopt);

  // The shape of the pixels of a `size` area for which `inside(x, y)` holds.
  template <class _Inside>
  [[nodiscard]] static auto Of(Size const size, _Inside inside) -> Bits {
    auto const width_bytes{ static_cast<std::size_t>(size.width + 7) / 8 };
    auto       bytes{ std::vector<std::byte>(width_bytes * static_cast<std::size_t>(size.height)) };
    std::ranges::for_each(assets::Grid(static_cast<std::size_t>(size.height), static_cast<std::size_t>(size.width)), [&](auto const place) {
      auto const [y, x]{ place };
      if (inside(static_cast<int>(x), static_cast<int>(y))) bytes.at(y * width_bytes + x / 8) |= std::byte{ static_cast<std::uint8_t>(0x80U >> (x % 8)) };
    });
    return Bits{ std::move(bytes), size };
  }

  // The same pixels with every bit flipped (the shipped asset they were is no longer theirs).
  [[nodiscard]] auto Inverted() const -> Bits;
  [[nodiscard]] auto Set(int x, int y) const -> bool;
  // Whether every 1 bit of `other`, placed `offset` pixels from these, falls on a 1 bit of these.
  [[nodiscard]] auto Holds(Bits const& other, Point offset) const -> bool;
  // The bytes as the texture store takes them.
  [[nodiscard]] auto Image() const -> GameImage;
  [[nodiscard]] auto Dimensions() const noexcept -> Size {
    return _size;
  }

private:
  [[nodiscard]] auto WidthBytes() const noexcept -> std::size_t {
    return static_cast<std::size_t>(_size.width + 7) / 8;
  }

  std::vector<std::byte>        _bytes;
  Size                          _size;
  std::optional<assets::Layout> _shipped;
};

using SharedBits = std::shared_ptr<Bits const>;

// An image the game drew from its bytes, kept to be drawn again: the bytes, how their planes lie, the shipped layout
// they may be, and how many lines each row is drawn on.
struct KeptImage {
  std::vector<std::byte>        bytes;
  assets::PlaneLayout           layout;
  std::optional<assets::Layout> shipped;
  std::size_t                   repeat{ 1 };

  [[nodiscard]] auto Image() const -> GameImage;
  [[nodiscard]] auto Dimensions() const noexcept -> Size {
    return Size{ .width = static_cast<int>(layout.width_bytes * 8), .height = static_cast<int>(layout.height * repeat) };
  }
};

using SharedImage   = std::shared_ptr<KeptImage const>;
using SharedPicture = std::shared_ptr<assets::DrawnPicture const>;

// ---- the steps, in frame pixels and colour numbers

struct FillStep {
  Rect         area;
  std::uint8_t number{};
};

struct PointsStep {
  std::vector<Point> points;
  Rect               bounds;
  std::uint8_t       number{};
};

// A shape's 1 bits in one colour number.
struct InkStep {
  SharedBits   shape;
  Rect         destination;
  std::uint8_t number{};
};

// An image, its numbers passed through `numbers` first (a copy that sets colour bit 3 shows n as n | 8).
struct ImageStep {
  SharedImage image;
  Point       at;
  Keying      keying{ Keying::OPAQUE };
  NumberMap   numbers{ SAME };
};

// A picture as its stream drew it, and the pack's picture over it when the stream is a shipped one.
struct PictureStep {
  SharedPicture  picture;
  assets::Layout stream{ assets::Layout::STREAM_8 };
  Point          at;
};

// The numbers under `area` (under the mask's 1 bits only, when there is a mask) shown through `map`: the game's XOR,
// AND and OR with the screen. `overlay` names the overlay it shows (0 for none).
struct RemapStep {
  Rect                      area;
  std::optional<SharedBits> mask;
  NumberMap                 map{ SAME };
  std::uint32_t             overlay{};
};

// An image combined with the numbers under it: under the pixels of each of its colours (`mask`) the numbers through that
// colour's lookup; the masks do not overlap. The game's XOR, AND and OR of a many-coloured image.
struct CombineStep {
  struct Part {
    SharedBits mask;
    NumberMap  map{ SAME };
  };

  Rect              area;
  std::vector<Part> parts;
};

using Step       = std::variant<FillStep, PointsStep, InkStep, ImageStep, PictureStep, RemapStep, CombineStep>;
using SharedStep = std::shared_ptr<Step const>;

// A step where it is drawn: moved by `offset` and changing only the pixels in `clip` (a step copied elsewhere).
struct Placed {
  SharedStep step;
  Point      offset{};
  Rect       clip{ LOGICAL_SCREEN };
};

// The pixels a step can change.
[[nodiscard]] auto Bounds(Step const& step) -> Rect;
[[nodiscard]] auto Bounds(Placed const& placed) -> Rect;
// The pixels it sets whatever was under them (none when it combines with them or lets them show).
[[nodiscard]] auto Covered(Placed const& placed) -> std::optional<Rect>;

[[nodiscard]] auto Intersect(Rect first, Rect second) noexcept -> Rect;
[[nodiscard]] auto Empty(Rect rect) noexcept -> bool;
[[nodiscard]] auto Contains(Rect outer, Rect inner) noexcept -> bool;

class Recipe {
public:
  // Adds a step at the end; the steps it covers whole are let go.
  auto Add(Placed placed) -> void;
  // The steps that touch `area`, in order: what a copy, a saved rectangle or a cell draws again.
  [[nodiscard]] auto Touching(Rect area) const -> Recipe;
  // These steps where the copy of `from` lands with its corner at `to`: moved, and clipped to the copy.
  [[nodiscard]] auto Copied(Rect from, Point to) const -> std::vector<Placed>;
  // Where the overlay step named `overlay` is, and letting go of a step.
  [[nodiscard]] auto Find(std::uint32_t overlay) const -> std::optional<std::size_t>;
  auto               Remove(std::size_t at) -> void;
  auto               Clear() -> void {
    _steps.clear();
  }

  [[nodiscard]] auto Steps() const noexcept -> std::span<Placed const> {
    return _steps;
  }

private:
  std::vector<Placed> _steps;
};

}

namespace opeu::render {
using detail::frame_steps::Bits;
using detail::frame_steps::Bounds;
using detail::frame_steps::CombineStep;
using detail::frame_steps::Contains;
using detail::frame_steps::Covered;
using detail::frame_steps::Empty;
using detail::frame_steps::FillStep;
using detail::frame_steps::ImageStep;
using detail::frame_steps::InkStep;
using detail::frame_steps::Intersect;
using detail::frame_steps::KeptImage;
using detail::frame_steps::PictureStep;
using detail::frame_steps::Placed;
using detail::frame_steps::PointsStep;
using detail::frame_steps::Recipe;
using detail::frame_steps::RemapStep;
using detail::frame_steps::SharedBits;
using detail::frame_steps::SharedImage;
using detail::frame_steps::SharedPicture;
using detail::frame_steps::SharedStep;
using detail::frame_steps::Step;
}
