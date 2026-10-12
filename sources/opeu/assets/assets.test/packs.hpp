#pragma once
// The sample of the classic pack in assets.test/packs/ (tools/assets/pack.py --only, kept current by
// tools/assets/pack.test), in its two forms, and packs made in memory. Tests run in the module directory.

#include "opeu/assets/pack-files.hpp"
#include "opeu/assets/pack.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <memory>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace opeu::assets::test {

inline constexpr auto SAMPLE_DIRECTORY{ std::string_view{ "assets.test/packs/classic-sample" } };
inline constexpr auto SAMPLE_ARCHIVE{ std::string_view{ "assets.test/packs/classic-sample.opeupack" } };

[[nodiscard]] inline auto ToBytes(std::string_view const text) -> std::vector<std::byte> {
  return text | std::views::transform([](char const c) { return static_cast<std::byte>(c); }) | std::ranges::to<std::vector>();
}

[[nodiscard]] inline auto OpenSample(std::string_view const path) -> Pack {
  auto pack{ OpenPack(path) };
  EXPECT_TRUE(pack) << path << ": " << Describe(pack.error());
  return *std::move(pack);
}

// A file of the sample's directory form.
[[nodiscard]] inline auto SampleFile(std::string_view const path) -> std::vector<std::byte> {
  return DirectoryPackFiles{ SAMPLE_DIRECTORY }.Read(path).value_or(std::vector<std::byte>{});
}

[[nodiscard]] inline auto OpenMemoryPack(MemoryPackFiles::Files files) -> Packed<Pack> {
  return Pack::Open(std::make_unique<MemoryPackFiles const>(std::move(files)));
}

}
