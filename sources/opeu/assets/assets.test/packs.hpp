#pragma once
// The packs the tests load. The synthetic pack (assets.test/synthetic-pack/: our own pixels, every feature of the
// format) and its remaster (assets.test/synthetic-remaster/) prove the loader in every clone. The classic pack the
// module embeds (packs.embed/classic/, checked in and kept current by tools/assets/pack.test) is in the private
// repository only: its directory, an archive of a few of its images made in memory, and the embedded files; a test of
// it skips without it (game-data.test.hpp). Tests run in the module directory.

#include "opeu/game-data.test.hpp"

#include "opeu/assets/embedded-pack.hpp"
#include "opeu/assets/pack-files.hpp"
#include "opeu/assets/pack.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <filesystem>
#include <iterator>
#include <memory>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

namespace opeu::assets::test {
using namespace std::string_literals;
using namespace std::string_view_literals;
inline constexpr auto CLASSIC_DIRECTORY{ "packs.embed/classic"sv };
inline constexpr auto SYNTHETIC_DIRECTORY{ "assets.test/synthetic-pack"sv };
inline constexpr auto SYNTHETIC_REMASTER{ "assets.test/synthetic-remaster"sv };

// A few assets covering every layout, the hour, ink and scene recolouring, a side-0 variant, an animation and all six
// fonts: what the tests load by id (the whole pack is tools/assets/pack.test's).
inline constexpr auto SAMPLE_IDS{ std::to_array<std::string_view>({
    "outcome/medal-0",
    "frame/strip-4",
    "opening/00",
    "view-tile/map-2/17",
    "battle-terrain/snow/70",
    "battle-unit/class-03/facing-1",
    "battle-unit/class-03/facing-1/side-0",
    "battle-blast/large/0",
    "battle-blast/large/1",
    "battle-blast/large/2",
    "battle-blast/large/3",
    "battle-border/nw-se",
    "portrait/118",
    "silhouette/012",
    "dialog/yes",
    "logo/presents",
    "ending-glyph/000",
    "game-glyph/0042",
    "text-glyph/065",
    "digit-glyph/07",
    "sideways-digit-glyph/03",
    "marker-glyph/1",
}) };

[[nodiscard]] inline auto ToBytes(std::string_view const text) -> std::vector<std::byte> {
  return text | std::views::transform([](char const c) { return static_cast<std::byte>(c); }) | std::ranges::to<std::vector>();
}

// A file of a pack's directory form.
[[nodiscard]] inline auto PackFile(std::string_view const directory, std::string_view const path) -> std::vector<std::byte> {
  return DirectoryPackFiles{ directory }.Read(path).value_or(std::vector<std::byte>{});
}

[[nodiscard]] inline auto ClassicFile(std::string_view const path) -> std::vector<std::byte> {
  return PackFile(CLASSIC_DIRECTORY, path);
}

[[nodiscard]] inline auto SyntheticFile(std::string_view const path) -> std::vector<std::byte> {
  return PackFile(SYNTHETIC_DIRECTORY, path);
}

[[nodiscard]] inline auto ClassicDirectory() -> Pack {
  return OpenPack(CLASSIC_DIRECTORY).value();
}

[[nodiscard]] inline auto ClassicEmbedded() -> Pack {
  return OpenEmbeddedPack(CLASSIC_PACK).value();
}

[[nodiscard]] inline auto SyntheticDirectory() -> Pack {
  return OpenPack(SYNTHETIC_DIRECTORY).value();
}

// The paths of the given assets' images and index maps in `pack`, sorted, pack.json first.
[[nodiscard]] inline auto PathsOf(Pack const& pack, std::span<std::string const> const ids) -> std::vector<std::string> {
  auto paths{ ids | std::views::transform([&](std::string_view const id) { return pack.Find(id).value(); })
              | std::views::transform([](PackedAsset const& entry) { return std::array{ entry.image, entry.indexed.value_or(std::string{}) }; }) | std::views::join
              | std::views::filter([](std::string const& path) { return !path.empty(); }) | std::ranges::to<std::vector>() };
  std::ranges::sort(paths);
  paths.insert(paths.begin(), std::string{ MANIFEST_PATH });
  return paths;
}

// The paths of the classic sample.
[[nodiscard]] inline auto SamplePaths() -> std::vector<std::string> {
  auto const ids{ SAMPLE_IDS | std::views::transform([](std::string_view const id) { return std::string{ id }; }) | std::ranges::to<std::vector>() };
  return PathsOf(ClassicDirectory(), ids);
}

// Every file of the synthetic pack.
[[nodiscard]] inline auto SyntheticPaths() -> std::vector<std::string> {
  auto const pack{ SyntheticDirectory() };
  return PathsOf(pack, pack.Ids());
}

// An .opeupack archive of the given directory and files' bytes.
[[nodiscard]] inline auto ArchiveOf(std::string_view const directory, std::vector<std::byte> const& body) -> std::vector<std::byte> {
  auto       bytes{ ToBytes("OPEUPACK") };
  auto const length{ directory.size() };
  std::ranges::transform(std::array{ 0, 8, 16, 24 }, std::back_inserter(bytes), [length](int const shift) { return static_cast<std::byte>(length >> shift & 0xFF); });
  std::ranges::copy(ToBytes(directory), std::back_inserter(bytes));
  std::ranges::copy(body, std::back_inserter(bytes));
  return bytes;
}

// Where a file's bytes lie in an archive's body.
struct Stored {
  std::size_t at;
  std::size_t bytes;
};

// One file's line of an archive's directory.
[[nodiscard]] inline auto Listed(std::string const& path, Stored const stored) -> nlohmann::json {
  auto line     = nlohmann::json::object(); // not braces: a braced json is an array holding it
  line["path"]  = path;
  line["at"]    = stored.at;
  line["bytes"] = stored.bytes;
  return line;
}

// The files `paths` of the pack directory `pack` as one archive, written as pack.py writes one.
[[nodiscard]] inline auto ArchiveOfFiles(std::string_view const pack, std::span<std::string const> const paths) -> std::vector<std::byte> {
  auto directory      = nlohmann::json::object(); // not braces, as above
  directory["format"] = "opeu-archive/1";
  directory["files"]  = nlohmann::json::array();
  auto body{ std::vector<std::byte>{} };
  std::ranges::for_each(paths, [&](std::string const& path) {
    auto const file{ PackFile(pack, path) };
    directory["files"].push_back(Listed(path, Stored{ .at = body.size(), .bytes = file.size() }));
    std::ranges::copy(file, std::back_inserter(body));
  });
  return ArchiveOf(directory.dump(), body);
}

[[nodiscard]] inline auto SampleArchive() -> std::vector<std::byte> {
  return ArchiveOfFiles(CLASSIC_DIRECTORY, SamplePaths());
}

[[nodiscard]] inline auto SyntheticArchiveBytes() -> std::vector<std::byte> {
  return ArchiveOfFiles(SYNTHETIC_DIRECTORY, SyntheticPaths());
}

[[nodiscard]] inline auto ArchivePack(std::vector<std::byte> bytes) -> Pack {
  auto files{ ArchivePackFiles::FromBytes(std::move(bytes)).value() };
  return Pack::Open(std::make_unique<ArchivePackFiles const>(std::move(files))).value();
}

[[nodiscard]] inline auto ClassicArchive() -> Pack {
  return ArchivePack(SampleArchive());
}

[[nodiscard]] inline auto SyntheticArchive() -> Pack {
  return ArchivePack(SyntheticArchiveBytes());
}

// A test of the classic pack as checked in: the private repository's only.
class CheckedInClassicTest : public ::testing::Test {
protected:
  auto SetUp() -> void override {
    if (!::opeu::test::TestFile(CLASSIC_DIRECTORY)) GTEST_SKIP() << ::opeu::test::NoGameData("the checked-in classic pack");
  }
};

[[nodiscard]] inline auto OpenMemoryPack(MemoryPackFiles::Files files) -> Packed<Pack> {
  return Pack::Open(std::make_unique<MemoryPackFiles const>(std::move(files)));
}

}
