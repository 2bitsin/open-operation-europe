// Loads a scenario: its record (the data segment from scenario_first through scenario_last) from the scenario file,
// rebased, then the map its scenario byte names from the map file.
#include "io/loadscen.hpp"

#include "common/data-segment.hpp"
#include "common/scenario.hpp"
#include "io/rebase.hpp"

#include <cstddef>
#include <cstring>
#include <utility>

using namespace opeu::game;

namespace {

constexpr auto        SCENARIO_COUNT{ 7u };
constexpr std::size_t SCENARIOS_AT{ 0x2580 }; // the records' offset in the scenario file
constexpr std::size_t RECORD_SIZE{ offsetof(DataSegment, scenario_last) + 1 - offsetof(DataSegment, scenario_first) };
constexpr std::size_t MAP_SIZE{ sizeof(DataSegment::map_cells) };
} // namespace

// Returns -1, leaving the data segment untouched, when a file is shorter than the records it reads.
auto opeu_load_scenario(std::uint8_t* ds, unsigned scenario, std::uint8_t const* scedata, std::size_t scedata_size, std::uint8_t const* mapinf, std::size_t mapinf_size) -> int {
  if (scenario >= SCENARIO_COUNT) return -1;
  auto const position{ SCENARIOS_AT + scenario * RECORD_SIZE };
  if (position > scedata_size || scedata_size - position < RECORD_SIZE) return -1;
  // the record's second byte is its scenario byte
  auto const map_position{ (scedata[position + 1] & SCENARIO_MASK) * MAP_SIZE };
  if (map_position > mapinf_size || mapinf_size - map_position < MAP_SIZE) return -1;
  auto& data{ Data(ds) };
  std::memcpy(ds + offsetof(DataSegment, scenario_first), scedata + position, RECORD_SIZE);
  opeu_rebase_scenario(ds, 0);
  std::memcpy(data.map_cells.data(), mapinf + std::to_underlying(CurrentScenario(data)) * MAP_SIZE, MAP_SIZE);
  return 0;
}
