#pragma once
// The source layer: beside the game's frame of colour numbers, where each 8x8 cell of it came from. A cell that an image
// of the pack wholly covers names that image, where in it the cell lies and the colours a one-bit image is drawn in;
// every other cell is raw, its numbers all there is. Any write that is not such an image makes the cells it touches
// raw; a copy moves the cells it moves whole with their pixels. docs/rendering.md, "The source layer", has the rule and
// the remaster present it is for.

#include "opeu/render/geometry.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace opeu::render::detail::source_layer {

inline constexpr auto CELL{ 8 };
// The first row of cells starts at frame line 4: the map view's 24-line tiles lie at line 4 mod 8, so each covers 3 x 3
// whole cells. The four lines above and below the grid are outside the game area (lines 40 to 439) and always raw.
inline constexpr auto CELL_TOP{ 4 };
inline constexpr auto CELL_COLUMNS{ LOGICAL_SIZE.width / CELL };
inline constexpr auto CELL_ROWS{ (LOGICAL_SIZE.height - CELL_TOP) / CELL };

// An image of the layer's names (1 up; 0 is raw numbers).
enum class ImageNumber : std::uint32_t {
  RAW,
};

// What a cell shows: raw numbers, or image `image` with the cell's top-left corner at `offset` in it; a one-bit image
// in `colours` (its ink in the low four bits, its paper in the high four; 0 for a colour image).
struct Source {
  ImageNumber  image{ ImageNumber::RAW };
  Point        offset;
  std::uint8_t colours{};

  auto operator==(Source const&) const -> bool = default;
};

// An image laid on the frame: which, its top-left corner (frame pixels), its colours.
struct Placed {
  ImageNumber  image{ ImageNumber::RAW };
  Point        at;
  std::uint8_t colours{};
};

// The cells of a rectangle as they were (row by row over the cells it touches), to go with a copy of its pixels.
struct KeptSources {
  Rect                area;
  std::vector<Source> cells;
};

// An XOR's cells: what they showed before it, and the write that made them raw (undone only if nothing wrote since).
struct Toggled {
  KeptSources   before;
  std::uint32_t write{};
};

class SourceLayer {
public:
  SourceLayer();

  // The number of image `id`, given on first use.
  [[nodiscard]] auto Number(std::string_view id) -> ImageNumber;
  [[nodiscard]] auto Name(ImageNumber image) const -> std::optional<std::string_view>;

  // `area` (frame pixels) shows `placed`: the cells it wholly covers name the image, the others it touches are raw.
  auto Place(Rect area, Placed placed) -> void;
  // The cells `area` touches are raw.
  auto Clear(Rect area) -> void;
  auto ClearAll() -> void;
  // The cells of `area`, kept to go with its pixels.
  [[nodiscard]] auto Keep(Rect area) const -> KeptSources;
  // The kept pixels put back with their top-left corner at `at`: a cell they cover whole, moved by whole cells, takes
  // its kept source; every other cell they touch is raw.
  auto Put(KeptSources const& kept, Point at) -> void;
  // `from` copied with its top-left corner at `to`: Put(Keep(from), to) without keeping anything.
  auto Move(Rect from, Point to) -> void;
  // An XOR over `area`: its cells raw, and what puts them back when the same XOR undoes it.
  [[nodiscard]] auto Toggle(Rect area) -> Toggled;
  auto               Untoggle(Toggled const& toggled) -> void;

  [[nodiscard]] auto At(Point cell) const -> Source;
  // The memory the layer holds besides its fixed grid: the names.
  [[nodiscard]] auto Footprint() const noexcept -> std::size_t;

private:
  // Names looked up by any string type.
  struct NameHash {
    using is_transparent = void;

    auto operator()(std::string_view const name) const noexcept -> std::size_t {
      return std::hash<std::string_view>{}(name);
    }
  };

  auto Write(Point cell, Source source) -> void;
  auto MoveCells(Rect from, Point to) -> void;

  std::vector<Source>                                                     _cells;
  std::vector<std::uint32_t>                                              _writes; // the write that last set each cell
  std::uint32_t                                                           _write{};
  std::vector<std::string>                                                _names;
  std::unordered_map<std::string, ImageNumber, NameHash, std::equal_to<>> _numbers;
  KeptSources                                                             _moving; // what Move copies, its storage reused
};

}

namespace opeu::render {
using detail::source_layer::CELL;
using detail::source_layer::CELL_COLUMNS;
using detail::source_layer::CELL_ROWS;
using detail::source_layer::CELL_TOP;
using detail::source_layer::ImageNumber;
using detail::source_layer::KeptSources;
using detail::source_layer::Placed;
using detail::source_layer::Source;
using detail::source_layer::SourceLayer;
using detail::source_layer::Toggled;
}
