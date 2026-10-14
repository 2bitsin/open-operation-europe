#pragma once
// The classic pack the module embeds (packs.embed/classic/, checked in and kept current by tools/assets/pack.test), in
// its three forms: the directory, an archive of a few of its images made in memory, and the embedded files. Tests run
// in the module directory.

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
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

namespace opeu::assets::test {

inline constexpr auto CLASSIC_DIRECTORY{ std::string_view{ "packs.embed/classic" } };

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

// A file of the classic pack's directory form.
[[nodiscard]] inline auto ClassicFile(std::string_view const path) -> std::vector<std::byte> {
  return DirectoryPackFiles{ CLASSIC_DIRECTORY }.Read(path).value_or(std::vector<std::byte>{});
}

[[nodiscard]] inline auto ClassicDirectory() -> Pack {
  return OpenPack(CLASSIC_DIRECTORY).value();
}

[[nodiscard]] inline auto ClassicEmbedded() -> Pack {
  return OpenEmbeddedPack(CLASSIC_PACK).value();
}

// The paths of the sample's images and index maps in the classic pack, sorted, pack.json first.
[[nodiscard]] inline auto SamplePaths() -> std::vector<std::string> {
  auto const classic{ ClassicDirectory() };
  auto       paths{ SAMPLE_IDS | std::views::transform([&](std::string_view const id) { return classic.Find(id).value(); })
                    | std::views::transform([](PackedAsset const& entry) { return std::array{ entry.image, entry.indexed.value_or(std::string{}) }; }) | std::views::join
                    | std::views::filter([](std::string const& path) { return !path.empty(); }) | std::ranges::to<std::vector>() };
  std::ranges::sort(paths);
  paths.insert(paths.begin(), std::string{ MANIFEST_PATH });
  return paths;
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

// One file's line of an archive's directory.
[[nodiscard]] inline auto Listed(std::string const& path, std::size_t const at, std::size_t const bytes) -> nlohmann::json {
  auto line     = nlohmann::json::object(); // not braces: a braced json is an array holding it
  line["path"]  = path;
  line["at"]    = at;
  line["bytes"] = bytes;
  return line;
}

// The classic pack's manifest and the sample's images as one archive, written as pack.py writes one.
[[nodiscard]] inline auto SampleArchive() -> std::vector<std::byte> {
  auto directory      = nlohmann::json::object(); // not braces, as above
  directory["format"] = "opeu-archive/1";
  directory["files"]  = nlohmann::json::array();
  auto body{ std::vector<std::byte>{} };
  std::ranges::for_each(SamplePaths(), [&](std::string const& path) {
    auto const file{ ClassicFile(path) };
    directory["files"].push_back(Listed(path, body.size(), file.size()));
    std::ranges::copy(file, std::back_inserter(body));
  });
  return ArchiveOf(directory.dump(), body);
}

[[nodiscard]] inline auto ClassicArchive() -> Pack {
  auto files{ ArchivePackFiles::FromBytes(SampleArchive()).value() };
  return Pack::Open(std::make_unique<ArchivePackFiles const>(std::move(files))).value();
}

[[nodiscard]] inline auto OpenMemoryPack(MemoryPackFiles::Files files) -> Packed<Pack> {
  return Pack::Open(std::make_unique<MemoryPackFiles const>(std::move(files)));
}

}
