#include "extractor.hpp"

#include "json.hpp"
#include "png.hpp"

#include "opeu/assets/maps.hpp"
#include "opeu/assets/palette.hpp"
#include "opeu/assets/scenario.hpp"

#include <algorithm>
#include <format>
#include <ranges>
#include <set>
#include <utility>
#include <vector>

namespace opeu::probes::extract_assets::detail::extractor {

namespace {

using assets::Asset;
using assets::Decoded;
using Done = std::expected<void, std::string>;

auto Failed(std::string_view const what, assets::DecodeError const error) -> Done {
  return std::unexpected{ std::format("extract-assets: {}: {}", what, assets::Describe(error)) };
}

// The indices an image draws, ascending.
auto ColoursUsed(assets::Image const& image) -> std::vector<int> {
  auto drawn{ std::views::iota(0uz, image.Pixels().size()) | std::views::filter([&image](std::size_t const n) {
                return image.Opaque().empty() || image.Opaque()[n] != 0;
              })
              | std::views::transform([&image](std::size_t const n) { return int{ image.Pixels()[n] }; })
              | std::ranges::to<std::set>() };
  return drawn | std::ranges::to<std::vector>();
}

// Rows of a square grid of bytes as a JSON array of arrays.
auto Grid(std::span<std::uint8_t const> const cells, std::size_t const side) -> std::string {
  return json::Array(std::views::iota(0uz, cells.size() / side) | std::views::transform([&](std::size_t const row) {
                       return json::Array(cells.subspan(row * side, side));
                     }));
}

// A game text field: ASCII as a string; text in KOEI's double-byte code (the Chinese edition's names in set-up 0)
// has no Unicode mapping here, so it goes out as null with its bytes in hex under <name>_koei.
auto GameText(json::Object& record, std::string_view const name, std::string_view const text) -> json::Object& {
  if (std::ranges::all_of(text, [](char const c) { return static_cast<unsigned char>(c) < 0x80; })) {
    return record.Text(name, text);
  }
  auto const hex{ text | std::views::transform([](char const c) {
                    return std::format("{:02x}", static_cast<unsigned char>(c));
                  })
                  | std::views::join | std::ranges::to<std::string>() };
  return record.Raw(name, "null").Text(std::format("{}_koei", name), hex);
}

auto AssetRecord(Asset const& asset, assets::Image const& image) -> json::Object {
  auto record{ json::Object{} };
  record.Text("kind", "asset")
      .Text("id", asset.id)
      .Text("file", asset.file)
      .Text("group", asset.group)
      .Number("at", asset.at)
      .Text("layout", assets::Spelling(asset.layout))
      .Text("role", assets::Spelling(asset.role))
      .Number("width", image.Width())
      .Number("height", image.Height())
      .Text("palette", asset.palette)
      .Flag("mask", image.HasMask())
      .Raw("colours", json::Array(ColoursUsed(image)))
      .Text("image", std::format("images/{}.png", asset.id))
      .Text("indices", std::format("images/{}.index.png", asset.id))
      .Text("use", asset.use);
  if (asset.layout == assets::Layout::BITMAP || asset.layout == assets::Layout::RUNS_BITMAP) {
    record.Number("ink", asset.ink).Flag("ink_where_clear", asset.ink_where_clear);
  }
  if (asset.hour_coloured) record.Flag("hour_coloured", true);
  if (!asset.animation.empty()) record.Text("animation", asset.animation).Number("frame", asset.frame);
  return record;
}

}

Extractor::Extractor(assets::Source const& source, std::filesystem::path out, Emit emit)
    : _source{ source }, _catalogue{ source }, _out{ std::move(out) }, _emit{ std::move(emit) } { }

auto Extractor::Run() const -> Done {
  return Palettes().and_then([this] { return Graphics(); }).and_then([this] { return Scenarios(); }).and_then([this] {
    return Maps();
  });
}

auto Extractor::Palettes() const -> Done {
  std::ranges::for_each(assets::ClassicPalettes(), [this](assets::NamedPalette const& palette) {
    auto colours{ assets::Colours(palette.levels) | std::views::transform([](assets::Colour const& c) {
                    return json::Array(std::array{ c.r, c.g, c.b });
                  })
                  | std::ranges::to<std::vector>() };
    _emit(json::Object{}
              .Text("kind", "palette")
              .Text("name", palette.name)
              .Text("use", palette.use)
              .Raw("levels", json::Array(palette.levels))
              .Raw("colours", json::Array(colours))
              .Encoded());
  });
  return {};
}

auto Extractor::Graphics() const -> Done {
  for (auto const& asset : _catalogue.Assets()) {
    if (auto const done{ Graphic(asset) }; !done) return done;
  }
  return {};
}

auto Extractor::Graphic(Asset const& asset) const -> Done {
  auto const image{ _catalogue.Decode(asset) };
  if (!image) return Failed(asset.id, image.error());
  auto const palette{ assets::FindPalette(asset.palette) };
  if (!palette) return std::unexpected{ std::format("extract-assets: {}: no palette {}", asset.id, asset.palette) };
  auto const base{ _out / "images" / asset.id };
  auto       record{ AssetRecord(asset, *image) };
  auto       done{ WriteColour(base.string() + ".png", *image, *palette).and_then([&] {
    return WriteIndices(base.string() + ".index.png", *image);
  }) };
  if (done && asset.side_coloured) {
    done = WriteColour(base.string() + ".side-0.png", assets::SideZeroColours(*image), *palette);
    record.Flag("side_coloured", true)
        .Raw("variants", json::Object{}.Text("side-0", std::format("images/{}.side-0.png", asset.id)).Encoded());
  }
  if (done) _emit(record.Encoded());
  return done;
}

auto Extractor::Scenarios() const -> Done {
  auto const file{ _source.get().File("SCEDATA").and_then(assets::DecodeScenarioFile) };
  if (!file) return Failed("SCEDATA", file.error());
  for (auto const n : std::views::iota(0uz, assets::detail::scenario::SCENARIOS)) {
    _emit(json::Object{}
              .Text("kind", "scenario-text")
              .Number("scenario", n)
              .Text("description", file->Description(n))
              .Raw("conditions",
                   json::Array(std::array{ json::Quoted(file->Condition(n, 0)), json::Quoted(file->Condition(n, 1)) }))
              .Encoded());
  }
  for (auto const n : std::views::iota(0uz, assets::detail::scenario::SETUPS)) {
    auto const& s{ file->SetupOf(n) };
    auto const  sides{
      s.sides | std::views::transform([](auto const& side) {
        return json::Object{}.Raw("stock", json::Array(side.stock)).Raw("income", json::Array(side.income)).Encoded();
      })
    };
    auto people{ std::views::zip(std::views::iota(0), s.people)
                 | std::views::filter([](auto const& p) { return !std::get<1>(p).name.empty(); })
                 | std::views::transform([](auto const& p) {
                     auto const& [index, person]{ p };
                     return GameText(json::Object{}.Number("index", index), "name", person.name)
                         .Number("side", person.side)
                         .Number("portrait", person.portrait)
                         .Raw("abilities", json::Array(person.abilities))
                         .Encoded();
                   }) };
    auto objectives{ std::views::zip(std::views::iota(0), s.objectives)
                     | std::views::filter([](auto const& o) { return !std::get<1>(o).name.empty(); })
                     | std::views::transform([](auto const& o) {
                         auto const& [index, objective]{ o };
                         return GameText(json::Object{}.Number("index", index), "name", objective.name)
                             .Number("x", objective.x)
                             .Number("y", objective.y)
                             .Number("garrison", objective.garrison)
                             .Number("points", objective.points)
                             .Number("flags", objective.flags)
                             .Encoded();
                       }) };
    auto units{ s.units | std::views::transform([](auto const& unit) {
                  return json::Object{}
                      .Number("first_element", unit.first_element)
                      .Number("x", unit.x)
                      .Number("y", unit.y)
                      .Raw("supplies", json::Array(unit.supplies))
                      .Number("flags", unit.flags)
                      .Encoded();
                }) };
    auto elements{ s.elements | std::views::transform([](auto const& e) {
                     return json::Object{}
                         .Number("next", e.next)
                         .Number("type", e.type)
                         .Number("quantity", e.quantity)
                         .Number("morale", e.morale)
                         .Number("training", e.training)
                         .Number("experience", e.experience)
                         .Number("state", e.state)
                         .Encoded();
                   }) };
    _emit(json::Object{}
              .Text("kind", "setup")
              .Number("setup", n)
              .Number("scenario", s.scenario)
              .Text("date", std::format("{:04}-{:02}-{:02}", s.year, s.month, s.day))
              .Number("hour", s.hour)
              .Number("weather", s.weather)
              .Number("difficulty", s.difficulty)
              .Raw("sides", json::Array(sides))
              .Raw("people", json::Array(people))
              .Raw("objectives", json::Array(objectives))
              .Raw("units", json::Array(units))
              .Raw("elements", json::Array(elements))
              .Encoded());
  }
  return {};
}

auto Extractor::Maps() const -> Done {
  auto const grids{ [this](std::string_view const name) {
    return _source.get().File(name).and_then(assets::DecodeCellGrids);
  } };
  auto const cells{ grids("MAPINF.DAT") };
  auto const tiles{ grids("MAPPNO.DAT") };
  auto const selectors{ grids("HEXDISP.DAT") };
  auto const fields{ _source.get().File("HEXMAP.DAT").and_then(assets::DecodeBattleFields) };
  if (!cells || !tiles || !selectors || !fields) return Failed("map layers", assets::DecodeError::BAD_SIZE);
  for (auto const map : std::views::iota(0uz, assets::MAP_COUNT)) {
    _emit(json::Object{}
              .Text("kind", "map")
              .Number("map", map)
              .Number("first_field", assets::FIRST_FIELD[map])
              .Raw("cells", Grid((*cells)[map], assets::MAP_SIDE))
              .Raw("tiles", Grid((*tiles)[map], assets::MAP_SIDE))
              .Raw("fields", Grid((*selectors)[map], assets::MAP_SIDE))
              .Encoded());
  }
  for (auto const [n, field] : std::views::zip(std::views::iota(0uz), *fields)) {
    _emit(json::Object{}
              .Text("kind", "battle-field")
              .Number("field", n)
              .Raw("cells", Grid(field, assets::FIELD_SIDE))
              .Encoded());
  }
  auto const token{ _source.get().File("ENDGRP.NPK") };
  if (!token) return Failed("ENDGRP.NPK", token.error());
  auto bytes{ token->subspan(assets::EPILOGUE_TOKEN_AT, assets::EPILOGUE_TOKEN_BYTES)
              | std::views::transform([](std::byte const b) { return std::to_integer<int>(b); }) };
  _emit(json::Object{}
            .Text("kind", "epilogue-token")
            .Number("at", assets::EPILOGUE_TOKEN_AT)
            .Raw("bytes", json::Array(bytes))
            .Encoded());
  return {};
}

}
