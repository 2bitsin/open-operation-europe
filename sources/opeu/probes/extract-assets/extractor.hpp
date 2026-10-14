#pragma once
// Writes every shipped asset to open formats: a PNG and an index PNG per graphic under <out>/images/, and one JSON
// record per line on the output stream for each asset and palette, one per table file (tables/*.json: the scenarios,
// map layers, battle fields, palettes and glyph codes, written by the assets module's table writers) and the epilogue's
// token; Texts adds texts/messages.json, the game's texts decoded by the code table. The music tables (music/*.json)
// come from the FM banks the recovered programs hold, each checked to read back into the same bank bytes and the same
// FM driver register stream; a music-check record per bank says so.

#include "opeu/assets/catalogue.hpp"
#include "opeu/assets/code-table.hpp"
#include "opeu/assets/music-table.hpp"
#include "opeu/assets/source.hpp"

#include <cstdio>
#include <expected>
#include <filesystem>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace opeu::probes::extract_assets::detail::extractor {

class Extractor {
public:
  using Emit = std::function<void(std::string_view record)>;

  Extractor(assets::Source const& source, std::filesystem::path out, Emit emit);

  // Everything, the game's texts through `table`.
  [[nodiscard]] auto Run(assets::CodeTable const& table) const -> std::expected<void, std::string>;

private:
  [[nodiscard]] auto Graphics() const -> std::expected<void, std::string>;
  [[nodiscard]] auto Graphic(assets::Asset const& asset) const -> std::expected<void, std::string>;
  [[nodiscard]] auto Palettes() const -> std::expected<void, std::string>;
  [[nodiscard]] auto Tables(assets::CodeTable const& table) const -> std::expected<void, std::string>;
  // The messages of every text the shipped files hold, as `table` decodes them.
  [[nodiscard]] auto Texts(assets::CodeTable const& table) const -> std::expected<void, std::string>;
  [[nodiscard]] auto EpilogueToken() const -> std::expected<void, std::string>;
  [[nodiscard]] auto Music() const -> std::expected<void, std::string>;
  [[nodiscard]] auto MusicTable(assets::MusicTable const& table, std::vector<std::byte> const& bank) const -> std::expected<void, std::string>;

  std::reference_wrapper<assets::Source const> _source;
  assets::Catalogue                            _catalogue;
  std::filesystem::path                        _out;
  Emit                                         _emit;
};

}

namespace opeu::probes::extract_assets {
using detail::extractor::Extractor;
}
