#pragma once
// SCEDATA as a table: tables/scenarios.json holds the scenario texts and the seven set-ups, every record of them,
// under the decoders' field names (docs/asset-pack.md, "Tables").

#include "opeu/assets/pack-error.hpp"
#include "opeu/assets/scenario.hpp"

#include <string>
#include <string_view>

namespace opeu::assets::detail::scenario_table {

inline constexpr auto SCENARIO_TABLE{ std::string_view{ "tables/scenarios.json" } };

// The table file of a scenario file.
[[nodiscard]] auto ScenarioTableText(ScenarioFile const& file) -> std::string;

// The scenario file a table file holds.
[[nodiscard]] auto ReadScenarioTable(Bytes bytes) -> Packed<ScenarioFile>;

}

namespace opeu::assets {
using detail::scenario_table::ReadScenarioTable;
using detail::scenario_table::SCENARIO_TABLE;
using detail::scenario_table::ScenarioTableText;
}
