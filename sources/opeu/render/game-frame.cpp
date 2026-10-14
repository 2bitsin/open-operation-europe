#include "opeu/render/game-frame.hpp"

#include "opeu/render/baking.hpp"
#include "opeu/render/colour-numbers.hpp"
#include "opeu/render/render-error.hpp"

#include <algorithm>
#include <ranges>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace opeu::render::detail::game_frame {
namespace {

constexpr auto TEXTURE_CAPACITY{ 4096uz };
constexpr auto CLEAR{
  Rgba{ .r = 0, .g = 0, .b = 0, .a = 0 }
};

// Colours that tell the sixteen numbers apart, for reading the numbers back.
constexpr auto NUMBER_COLOURS{ [] {
  auto colours{ assets::PaletteColours{} };
  std::ranges::transform(SAME, colours.begin(), [](std::uint8_t const n) {
    auto const level{ static_cast<std::uint8_t>(n * 17) };
    return assets::Colour{ .r = level, .g = level, .b = level };
  });
  return colours;
}() };

[[nodiscard]] constexpr auto Moved(Rect const rect, Point const by) noexcept -> Rect {
  return Rect{ .x = rect.x + by.x, .y = rect.y + by.y, .width = rect.width, .height = rect.height };
}

[[nodiscard]] constexpr auto Moved(Point const at, Point const by) noexcept -> Point {
  return Point{ .x = at.x + by.x, .y = at.y + by.y };
}

[[nodiscard]] constexpr auto Origin(Rect const rect) noexcept -> Point {
  return Point{ .x = rect.x, .y = rect.y };
}

[[nodiscard]] constexpr auto Away(Point const from, Point const to) noexcept -> Point {
  return Point{ .x = to.x - from.x, .y = to.y - from.y };
}

[[nodiscard]] auto ShapePaint() -> Paint {
  return Paint{ .colours = INK, .keying = Keying::OPAQUE };
}

// Whether two masks are the same shape (the same bytes kept once, or equal bytes).
[[nodiscard]] auto SameMask(std::optional<SharedBits> const& first, std::optional<SharedBits> const& second) -> bool {
  if (first.has_value() != second.has_value()) return false;
  if (!first) return true;
  return *first == *second
         || ((*first)->Dimensions() == (*second)->Dimensions() && (*first)->Image().bytes.size() == (*second)->Image().bytes.size()
             && std::ranges::equal((*first)->Image().bytes, (*second)->Image().bytes));
}

[[nodiscard]] auto SameRemap(RemapStep const& first, RemapStep const& second) -> bool {
  return first.area == second.area && first.map == second.map && SameMask(first.mask, second.mask);
}

}

GameFrame::GameFrame(Display display, assets::AssetPack const& pack, assets::ContentIndex const& index, assets::PaletteColours const& colours)
    : _display{ std::move(display) }, _pack_textures{ _display.Renderer(), pack, TEXTURE_CAPACITY }, _textures{ _display.Renderer(), _pack_textures, index, TEXTURE_CAPACITY }, _colours{ colours } {
  _display.Begin();
  Restart(colours);
}

auto GameFrame::Recolour(assets::PaletteColours const& colours) -> void {
  if (colours == _colours) return;
  _colours = colours;
  Repaint(LOGICAL_SCREEN);
}

auto GameFrame::Restart(assets::PaletteColours const& colours) -> void {
  _colours = colours;
  _recipe.Clear();
  Add(FillStep{ .area = LOGICAL_SCREEN, .number = 0 });
}

auto GameFrame::Add(Step step) -> void {
  Painted(Placed{ .step = std::make_shared<Step const>(std::move(step)), .offset = {}, .clip = LOGICAL_SCREEN });
  _display.Flush();
}

auto GameFrame::AddCopy(Recipe const& source, Rect const from, Point const to) -> void {
  std::ranges::for_each(source.Copied(from, to), [this](Placed const& placed) { Painted(placed); });
  _display.Flush();
}

// The step added and painted over what is there; SDL's queue of drawing stays one step long.
auto GameFrame::Painted(Placed const& placed) -> void {
  _recipe.Add(placed);
  _changed = true;
  auto const steps{ _recipe.Steps() };
  if (steps.empty() || steps.back().step != placed.step) return;
  PaintStep(steps.first(steps.size() - 1), steps.back(), Lens{ .offset = {}, .clip = LOGICAL_SCREEN, .map = SAME });
  _display.Unclip();
}

auto GameFrame::Withdraw(std::uint32_t const overlay) -> void {
  auto const at{ _recipe.Find(overlay) };
  if (!at) return;
  auto const area{ Bounds(_recipe.Steps()[*at]) };
  _recipe.Remove(*at);
  Repaint(area);
}

// Back from the end over the steps that touch its area: an identical XOR is taken out; another XOR does not stand in
// the way (XORs undo themselves in any order); any other step does.
auto GameFrame::WithdrawLast(RemapStep const& step) -> bool {
  if (!Exchanges(step.map)) return false;
  auto const steps{ _recipe.Steps() };
  auto const remap{ [&steps](std::size_t const n) -> std::optional<RemapStep> {
    auto const& placed{ steps[n] };
    if (!std::holds_alternative<RemapStep>(*placed.step) || placed.offset != Point{} || placed.clip != LOGICAL_SCREEN) return std::nullopt;
    return std::get<RemapStep>(*placed.step);
  } };
  auto       touching{ std::views::iota(0uz, steps.size()) | std::views::reverse | std::views::filter([&](std::size_t const n) { return !Empty(Intersect(Bounds(steps[n]), step.area)); }) };
  auto const blocking{ std::ranges::find_if(touching, [&](std::size_t const n) {
    auto const found{ remap(n) };
    return !found || !Exchanges(found->map) || SameRemap(*found, step);
  }) };
  if (blocking == touching.end()) return false;
  auto const found{ remap(*blocking) };
  if (!found || !SameRemap(*found, step)) return false;
  _recipe.Remove(*blocking);
  Repaint(step.area);
  return true;
}

auto GameFrame::Present(Rgba const tint) -> void {
  _display.Present(tint);
  _display.Resume();
}

auto GameFrame::Capture(Rgba const tint) -> assets::Texture {
  return Required(_display.Capture(tint));
}

auto GameFrame::Numbers() -> assets::Image const& {
  if (!_changed) return _numbers;
  auto layer{ Required(_display.CreateLayer(LOGICAL_SIZE)) };
  auto shown{ std::exchange(_colours, NUMBER_COLOURS) };
  _numbering = true;
  auto read{ [&] {
    auto const into{ _display.Into(layer) };
    PaintSteps(_recipe.Steps(), Lens{ .offset = {}, .clip = LOGICAL_SCREEN, .map = SAME });
    _display.Unclip();
    return Required(_display.Read(LOGICAL_SCREEN));
  }() };
  _colours   = shown;
  _numbering = false;
  _numbers   = NumbersOf(read, NUMBER_COLOURS);
  _changed   = false;
  return _numbers;
}

// ---- painting -----------------------------------------------------------------------------------------------------

auto GameFrame::Repaint(Rect const area) -> void {
  _changed = true;
  PaintSteps(_recipe.Steps(), Lens{ .offset = {}, .clip = Intersect(area, LOGICAL_SCREEN), .map = SAME });
  _display.Unclip();
  _display.Flush();
}

auto GameFrame::PaintSteps(std::span<Placed const> const steps, Lens const& lens) -> void {
  if (Empty(lens.clip)) return;
  std::ranges::for_each(std::views::iota(0uz, steps.size()), [&](std::size_t const n) {
    if (Empty(Intersect(Moved(Bounds(steps[n]), lens.offset), lens.clip))) return;
    PaintStep(steps.first(n), steps[n], lens);
  });
}

// The step where it is placed; what it combines with is painted from `before` with the lens it came with.
auto GameFrame::PaintStep(std::span<Placed const> const before, Placed const& placed, Lens const& lens) -> void {
  auto const own{
    Lens{ .offset = Moved(lens.offset, placed.offset), .clip = Intersect(lens.clip, Moved(placed.clip, lens.offset)), .map = lens.map }
  };
  if (Empty(own.clip)) return;
  auto const under{
    Lens{ .offset = lens.offset, .clip = own.clip, .map = lens.map }
  };
  _display.Clip(own.clip);
  std::visit(
      [&](auto const& drawn) {
        using Drawn = std::remove_cvref_t<decltype(drawn)>;
        if constexpr (std::is_same_v<Drawn, FillStep>) PaintFill(drawn, own);
        if constexpr (std::is_same_v<Drawn, PointsStep>) PaintPoints(drawn, own);
        if constexpr (std::is_same_v<Drawn, InkStep>) PaintInk(drawn, own);
        if constexpr (std::is_same_v<Drawn, ImageStep>) PaintImage(drawn, own);
        if constexpr (std::is_same_v<Drawn, PictureStep>) PaintPicture(drawn, own);
        if constexpr (std::is_same_v<Drawn, RemapStep>) PaintRemap(before, Remap{ .area = Moved(drawn.area, placed.offset), .mask = drawn.mask, .map = drawn.map }, under);
        if constexpr (std::is_same_v<Drawn, CombineStep>) PaintCombine(before, Combine{ .area = Moved(drawn.area, placed.offset), .parts = drawn.parts }, under);
      },
      *placed.step);
}

auto GameFrame::ColourOf(std::uint8_t const number, NumberMap const& map) const -> Rgba {
  return Pen(_colours, map.at(number & 0x0FU));
}

auto GameFrame::PaintFill(FillStep const& step, Lens const& lens) -> void {
  _display.Fill(Moved(step.area, lens.offset), ColourOf(step.number, lens.map));
}

auto GameFrame::PaintPoints(PointsStep const& step, Lens const& lens) -> void {
  auto const points{ step.points | std::views::transform([&lens](Point const at) { return Moved(at, lens.offset); }) | std::ranges::to<std::vector>() };
  _display.Plot(points, ColourOf(step.number, lens.map));
}

auto GameFrame::PaintInk(InkStep const& step, Lens const& lens) -> void {
  auto const shape{ Required(_textures.Fetch(step.shape->Image(), ShapePaint())) };
  _display.Draw(shape->texture, Blit{ .destination = Moved(step.destination, lens.offset), .modulation = ColourOf(step.number, lens.map) });
}

auto GameFrame::PaintImage(ImageStep const& step, Lens const& lens) -> void {
  auto const paint{
    Paint{ .colours = Through(_colours, Then(step.numbers, lens.map)), .keying = step.keying }
  };
  auto image{ step.image->Image() };
  if (_numbering) image.shipped = std::nullopt;
  auto const baked{ Required(_textures.Fetch(image, paint)) };
  _display.Draw(baked->texture, Blit{ .destination = At(Moved(step.at, lens.offset), baked->size) });
}

auto GameFrame::PaintPicture(PictureStep const& step, Lens const& lens) -> void {
  auto const paint{
    Paint{ .colours = Through(_colours, lens.map), .keying = Keying::OPAQUE }
  };
  auto const at{ Moved(step.at, lens.offset) };
  auto const drawn{ Required(_textures.Fetch(*step.picture, paint)) };
  _display.Draw(drawn->texture, Blit{ .destination = At(at, drawn->size) });
  if (_numbering) return;
  if (auto const shipped{ _textures.FetchShipped(*step.picture, step.stream, paint) }) _display.Draw((*shipped)->texture, Blit{ .destination = At(at, (*shipped)->size) });
}

// What is under the area painted again through the lookup: a solid colour when every number becomes the same one;
// straight onto the target for a whole rectangle; through the mask's layers otherwise.
auto GameFrame::PaintRemap(std::span<Placed const> const before, Remap const& remap, Lens const& lens) -> void {
  auto const map{ Then(remap.map, lens.map) };
  auto const area{ Moved(remap.area, lens.offset) };
  if (Constant(map) && remap.mask) {
    auto const shape{ Required(_textures.Fetch((*remap.mask)->Image(), ShapePaint())) };
    _display.Draw(shape->texture, Blit{ .destination = area, .modulation = Pen(_colours, map.front()) });
  } else if (Constant(map)) {
    _display.Fill(area, Pen(_colours, map.front()));
  } else if (remap.mask) {
    PaintThroughMask(before, remap, lens);
  } else {
    PaintSteps(before, Lens{ .offset = lens.offset, .clip = Intersect(area, lens.clip), .map = map });
  }
}

// Each colour's part, its masks apart from each other's.
auto GameFrame::PaintCombine(std::span<Placed const> const before, Combine const& combine, Lens const& lens) -> void {
  std::ranges::for_each(combine.parts, [&](CombineStep::Part const& part) {
    _display.Clip(lens.clip);
    PaintRemap(before, Remap{ .area = combine.area, .mask = part.mask, .map = part.map }, lens);
  });
}

// What is under the mask painted through the lookup into a layer, cut to the mask by multiplying it into the mask's
// white, and laid over the target.
auto GameFrame::PaintThroughMask(std::span<Placed const> const before, Remap const& remap, Lens const& lens) -> void {
  auto const area{ Intersect(Moved(remap.area, lens.offset), lens.clip) };
  if (Empty(area)) return;
  auto const room{ At({}, Dimensions(area)) };
  auto const shape{ Required(_textures.Fetch((*remap.mask)->Image(), ShapePaint())) };
  auto&      layers{ LayersAt(_depth) };
  auto const toward{ Away(Origin(area), {}) };
  {
    auto const into{ _display.Into(layers.under) };
    _display.Unclip();
    _display.Clear(CLEAR);
    ++_depth;
    PaintSteps(before, Lens{ .offset = Moved(lens.offset, toward), .clip = room, .map = Then(remap.map, lens.map) });
    --_depth;
    _display.Unclip();
  }
  {
    auto const into{ _display.Into(layers.shape) };
    _display.Unclip();
    _display.Clear(CLEAR);
    _display.Draw(shape->texture, Blit{ .destination = Moved(Moved(remap.area, lens.offset), toward) });
    _display.Draw(layers.under, Blit{ .destination = room, .source = room, .blending = Blending::MULTIPLY });
  }
  _display.Clip(lens.clip);
  _display.Draw(layers.shape, Blit{ .destination = area, .source = room });
}

auto GameFrame::LayersAt(std::size_t const depth) -> Layers& {
  while (_layers.size() <= depth) { // one pair per depth, made the first time it is reached
    _layers.push_back(Layers{ .under = Required(_display.CreateLayer(LOGICAL_SIZE)), .shape = Required(_display.CreateLayer(LOGICAL_SIZE)) });
  }
  return _layers.at(depth);
}

}
