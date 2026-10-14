#pragma once
// Writes every shipped asset to open formats: a PNG and an index PNG per graphic under <out>/images/, and one JSON
// record per line on the output stream for each asset and palette, one per table file (tables/*.json: the scenarios,
// map layers, battle fields, palettes and glyph codes, written by the assets module's table writers) and the epilogue's
// token.

#include "opeu/assets/catalogue.hpp"
#include "opeu/assets/source.hpp"

#include <cstdio>
#include <expected>
#include <filesystem>
#include <functional>
#include <string>
#include <string_view>

namespace opeu::probes::extract_assets::detail::extractor {

class Extractor {
public:
  using Emit = std::function<void(std::string_view record)>;

  Extractor(assets::Source const& source, std::filesystem::path out, Emit emit);

  [[nodiscard]] auto Run() const -> std::expected<void, std::string>;

private:
  [[nodiscard]] auto Graphics() const -> std::expected<void, std::string>;
  [[nodiscard]] auto Graphic(assets::Asset const& asset) const -> std::expected<void, std::string>;
  [[nodiscard]] auto Palettes() const -> std::expected<void, std::string>;
  [[nodiscard]] auto Tables() const -> std::expected<void, std::string>;
  [[nodiscard]] auto EpilogueToken() const -> std::expected<void, std::string>;

  std::reference_wrapper<assets::Source const> _source;
  assets::Catalogue                            _catalogue;
  std::filesystem::path                        _out;
  Emit                                         _emit;
};

}

namespace opeu::probes::extract_assets {
using detail::extractor::Extractor;
}
