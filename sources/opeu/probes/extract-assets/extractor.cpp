#include "extractor.hpp"

#include "json.hpp"
#include "png.hpp"

#include "opeu/assets/glyph-code-table.hpp"
#include "opeu/assets/glyphs.hpp"
#include "opeu/assets/map-tables.hpp"
#include "opeu/assets/maps.hpp"
#include "opeu/assets/messages.hpp"
#include "opeu/assets/palette-table.hpp"
#include "opeu/assets/palette.hpp"
#include "opeu/assets/scenario-table.hpp"
#include "opeu/assets/scenario.hpp"

#include <algorithm>
#include <format>
#include <ranges>
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
  auto drawn{ std::views::iota(0uz, image.Pixels().size()) | std::views::filter([&image](std::size_t const n) { return image.Opaque().empty() || image.Opaque()[n] != 0; })
              | std::views::transform([&image](std::size_t const n) { return int{ image.Pixels()[n] }; }) | std::ranges::to<std::vector>() };
  std::ranges::sort(drawn);
  auto const repeated{ std::ranges::unique(drawn) };
  drawn.erase(repeated.begin(), repeated.end());
  return drawn;
}

// One table file as a record: its path in the pack and its JSON.
auto TableRecord(std::string_view const path, std::string const& table) -> std::string {
  return json::Object{}.Text("kind", "table").Text("path", path).Raw("table", table).Encoded();
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

Extractor::Extractor(assets::Source const& source, std::filesystem::path out, Emit emit) : _source{ source }, _catalogue{ source }, _out{ std::move(out) }, _emit{ std::move(emit) } { }

auto Extractor::Run() const -> Done {
  return Palettes().and_then([this] { return Graphics(); }).and_then([this] { return Tables(); });
}

auto Extractor::Palettes() const -> Done {
  std::ranges::for_each(assets::ClassicPalettes(), [this](assets::NamedPalette const& palette) {
    auto colours{ assets::Colours(palette.levels) | std::views::transform([](assets::Colour const& c) { return json::Array(std::array{ c.r, c.g, c.b }); }) | std::ranges::to<std::vector>() };
    _emit(json::Object{}.Text("kind", "palette").Text("name", palette.name).Text("use", palette.use).Raw("levels", json::Array(palette.levels)).Raw("colours", json::Array(colours)).Encoded());
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
  auto       done{ WriteTexture(base.string() + ".png", assets::ToTexture(*image, *palette)).and_then([&] { return WriteIndices(base.string() + ".index.png", *image); }) };
  if (done && asset.side_coloured) {
    done = WriteTexture(base.string() + ".side-0.png", assets::ToTexture(assets::SideZeroColours(*image), *palette));
    record.Flag("side_coloured", true).Raw("variants", json::Object{}.Text("side-0", std::format("images/{}.side-0.png", asset.id)).Encoded());
  }
  if (done) _emit(record.Encoded());
  return done;
}

auto Extractor::Tables() const -> Done {
  auto const file{ _source.get().File("SCEDATA").and_then(assets::DecodeScenarioFile) };
  if (!file) return Failed("SCEDATA", file.error());
  auto const grids{ [this](std::string_view const name) { return _source.get().File(name).and_then(assets::DecodeCellGrids); } };
  auto const cells{ grids("MAPINF.DAT") };
  auto const tiles{ grids("MAPPNO.DAT") };
  auto const selectors{ grids("HEXDISP.DAT") };
  auto const fields{ _source.get().File("HEXMAP.DAT").and_then(assets::DecodeBattleFields) };
  if (!cells || !tiles || !selectors || !fields) return Failed("map layers", assets::DecodeError::BAD_SIZE);
  _emit(TableRecord(assets::SCENARIO_TABLE, assets::ScenarioTableText(*file)));
  _emit(TableRecord(assets::MAP_TABLE, assets::MapTableText(assets::MapLayers{ .cells = *cells, .tiles = *tiles, .battle_fields = *selectors })));
  _emit(TableRecord(assets::BATTLE_FIELD_TABLE, assets::BattleFieldTableText(*fields)));
  _emit(TableRecord(assets::PALETTE_TABLE, assets::PaletteTableText(assets::ClassicPalettes())));
  _emit(TableRecord(assets::GLYPH_CODE_TABLE, assets::GlyphCodeTableText(assets::EndingGlyphCodes())));
  return EpilogueToken();
}

auto Extractor::Texts(assets::CodeTable const& table) const -> Done {
  auto const file{ _source.get().File("SCEDATA").and_then(assets::DecodeScenarioFile) };
  if (!file) return Failed("SCEDATA", file.error());
  _emit(TableRecord(assets::MESSAGES, assets::MessagesText(assets::MessagesOf(assets::ScenarioTexts(*file), table))));
  return {};
}

auto Extractor::EpilogueToken() const -> Done {
  auto const token{ _source.get().File("ENDGRP.NPK") };
  if (!token) return Failed("ENDGRP.NPK", token.error());
  auto const bytes{ token->subspan(assets::EPILOGUE_TOKEN_AT, assets::EPILOGUE_TOKEN_BYTES) | std::views::transform([](std::byte const b) { return std::to_integer<int>(b); }) };
  _emit(json::Object{}.Text("kind", "epilogue-token").Number("at", assets::EPILOGUE_TOKEN_AT).Raw("bytes", json::Array(bytes)).Encoded());
  return {};
}

}
