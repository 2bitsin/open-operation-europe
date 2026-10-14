#include "opeu/assets/glyph-code-table.hpp"

#include "opeu/assets/glyphs.hpp"
#include "opeu/assets/table-json.hpp"

#include <ranges>

namespace opeu::assets::detail::glyph_code_table {

namespace {

using namespace table_json;

auto CodesOf(Json const& table) -> std::vector<std::uint32_t> {
  return Numbers<ENDING_GLYPHS>(table, "ending", 0xFFFF) | std::ranges::to<std::vector>();
}

}

auto GlyphCodeTableText(std::span<std::uint32_t const> const ending) -> std::string {
  auto table      = Json::object();
  table["ending"] = Json(ending | std::ranges::to<std::vector>());
  return TableText("glyph_codes", std::move(table));
}

auto ReadGlyphCodeTable(Bytes const bytes) -> Packed<std::vector<std::uint32_t>> {
  return ReadTable(bytes, "glyph_codes", CodesOf);
}

}
