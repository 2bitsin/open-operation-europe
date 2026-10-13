// Loads a scenario: its record (the data segment from scenario_first through scenario_last) from the scenario file,
// rebased, then the map its scenario byte names from the map file.
#include "io/loadscen.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/scenario.hpp"
#include "io/rebase.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <span>
#include <utility>

using namespace opeu::game;

namespace {

constexpr auto        SCENARIO_COUNT{ 7u };
constexpr std::size_t SCENARIOS_AT{ 0x2580 }; // the records' offset in the scenario file
constexpr std::size_t RECORD_SIZE{ offsetof(DataSegment, scenario_last) + 1 - offsetof(DataSegment, scenario_first) };
constexpr std::size_t MAP_SIZE{ sizeof(DataSegment::map_cells) };

using FileBytes = std::span<std::uint8_t const>;

// The `size` bytes at `position` of a file, when the file holds them.
auto Slice(FileBytes file, std::size_t position, std::size_t size) -> std::optional<FileBytes> {
  if (position > file.size() || file.size() - position < size) return std::nullopt;
  return file.subspan(position, size);
}
} // namespace

// Returns -1, leaving the data segment untouched, when a file is shorter than the records it reads.
auto opeu_load_scenario(Game& game, unsigned scenario, std::uint8_t const* scedata, std::size_t scedata_size, std::uint8_t const* mapinf, std::size_t mapinf_size) -> int {
  if (scenario >= SCENARIO_COUNT) return -1;
  auto const record{ Slice(FileBytes{ scedata, scedata_size }, SCENARIOS_AT + scenario * RECORD_SIZE, RECORD_SIZE) };
  if (!record) return -1;
  // the record's second byte is its scenario byte
  auto const map{ Slice(FileBytes{ mapinf, mapinf_size }, ((*record)[1] & SCENARIO_MASK) * MAP_SIZE, MAP_SIZE) };
  if (!map) return -1;
  auto& data{ Data(game) };
  std::ranges::copy(*record, game.State().Bytes(offsetof(DataSegment, scenario_first), RECORD_SIZE).begin());
  opeu_rebase_scenario(game, 0);
  std::ranges::copy(FileBytes{ mapinf, mapinf_size }.subspan(std::to_underlying(CurrentScenario(data)) * MAP_SIZE, MAP_SIZE), data.map_cells.begin());
  return 0;
}
