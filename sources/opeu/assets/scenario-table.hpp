#pragma once
// SCEDATA as a table: tables/scenarios.json holds the scenario texts and the seven set-ups, every record of them,
// under the decoders' field names (docs/asset-pack.md, "Tables"). Set-up 0's names in KOEI's code are written as UTF-8
// through the code table (texts/code-table.json), which also reads them back.

#include "opeu/assets/code-table.hpp"
#include "opeu/assets/pack-error.hpp"
#include "opeu/assets/scenario.hpp"

#include <string>
#include <string_view>

namespace opeu::assets::detail::scenario_table {

inline constexpr auto SCENARIO_TABLE{ std::string_view{ "tables/scenarios.json" } };

// The table file of a scenario file.
[[nodiscard]] auto ScenarioTableText(ScenarioFile const& file, CodeTable const& table) -> std::string;

// The scenario file a table file holds.
[[nodiscard]] auto ReadScenarioTable(Bytes bytes, CodeTable const& table) -> Packed<ScenarioFile>;

}

namespace opeu::assets {
using detail::scenario_table::ReadScenarioTable;
using detail::scenario_table::SCENARIO_TABLE;
using detail::scenario_table::ScenarioTableText;
}
