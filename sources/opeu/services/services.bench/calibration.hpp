// A fixed workload that no game code touches, benchmarked beside the game's: the bench gates divide the machine's
// momentary speed out of the other benchmarks with it (sources/opeu/integration/perf.test/perf.py). It mixes
// arithmetic and a 256 KiB working set, as the drawing and battle benchmarks do. Included by one source of each
// benchmark executable.
#pragma once

#include <benchmark/benchmark.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <numeric>

namespace opeu::bench {

inline constexpr auto CALIBRATION_WORDS{ 65536uz };

inline auto Calibration(benchmark::State& state) -> void {
  auto words{ std::make_unique<std::array<std::uint32_t, CALIBRATION_WORDS>>() };
  std::iota(words->begin(), words->end(), 1u);
  for (auto _ : state) {
    std::ranges::transform(*words, words->begin(), [](std::uint32_t word) {
      word ^= word << 13;
      word ^= word >> 17;
      return word ^ word << 5;
    });
    benchmark::DoNotOptimize(std::ranges::fold_left(*words, 0u, std::bit_xor{}));
  }
}
BENCHMARK(Calibration);

} // namespace opeu::bench
