// What the build compiled in: the 26 data files as the table describes them, and the classic pack byte for byte as
// checked in. A build without them (the public mirror) has neither, and these cases skip.
#include "embedded.hpp"

#include "opeu/game-data.test.hpp"

#include "opeu/assets/data-files.hpp"
#include "opeu/assets/fingerprint.hpp"
#include "opeu/assets/layered-pack.hpp"
#include "opeu/assets/pack-files.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <memory>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace opeu::app {
namespace {
using namespace std::string_view_literals;

inline constexpr auto CLASSIC_DIRECTORY{ "packs.embed/classic"sv };

class CompiledData : public ::testing::Test {
protected:
  auto SetUp() -> void override {
    auto table{ ::opeu::app::EmbeddedData() };
    if (!table.has_value()) GTEST_SKIP() << test::NoGameData("the data files compiled into this build");
    _table = std::make_unique<assets::GameData::Table const>(*std::move(table));
  }
  [[nodiscard]] auto Table() const -> assets::GameData::Table const& {
    return *_table;
  }

private:
  std::unique_ptr<assets::GameData::Table const> _table; // set by SetUp, or the test skipped
};

class CompiledPack : public ::testing::Test {
protected:
  auto SetUp() -> void override {
    if (!OpenEmbeddedPack(CLASSIC_PACK) || !test::TestFile(CLASSIC_DIRECTORY)) GTEST_SKIP() << test::NoGameData("the classic pack compiled into this build");
  }
};

// Every file of the checked-in pack, by its path under the pack's directory.
auto ClassicPaths() -> std::vector<std::string> {
  return std::filesystem::recursive_directory_iterator{ CLASSIC_DIRECTORY } | std::views::filter([](std::filesystem::directory_entry const& entry) { return entry.is_regular_file(); })
         | std::views::transform([](std::filesystem::directory_entry const& entry) { return entry.path().lexically_relative(CLASSIC_DIRECTORY).generic_string(); }) | std::ranges::to<std::vector>();
}

}

TEST_F(CompiledData, AreTheTwentySixFilesTheTableDescribes) {
  ASSERT_EQ(Table().size(), assets::DATA_FILES.size());
  std::ranges::for_each(assets::DATA_FILES, [this](assets::DataFile const& file) {
    auto const bytes{ std::as_bytes(Table().at(std::string{ file.name })) };
    EXPECT_EQ(bytes.size(), file.size) << file.name;
    EXPECT_EQ(assets::FingerprintOf(bytes), file.fingerprint) << file.name;
  });
}

TEST_F(CompiledData, AnswerTheFileLayersDosNames) {
  auto const data{ assets::GameData::FromTable(Table()) };
  EXPECT_EQ(data.DosFile("C:\\OPEU\\Msg.16p").value_or(std::span<std::uint8_t const>{}).size(), 30240U);
  EXPECT_EQ(data.DosFile("data/face.dat").value_or(std::span<std::uint8_t const>{}).size(), 230400U);
  EXPECT_FALSE(data.DosFile("SAVEDATA"));
  EXPECT_FALSE(data.DosFile("MAIN.EXE"));
}

TEST_F(CompiledPack, HoldsEveryFileOfTheCheckedInClassicPack) {
  auto const embedded{ EmbeddedPackFiles(CLASSIC_PACK) };
  auto const directory{ assets::DirectoryPackFiles{ CLASSIC_DIRECTORY } };
  auto const paths{ ClassicPaths() };
  EXPECT_GT(paths.size(), 6'000U);
  auto const differing{ paths | std::views::filter([&](std::string const& path) { return embedded->Read(path) != directory.Read(path); }) | std::ranges::to<std::vector>() };
  EXPECT_EQ(differing, std::vector<std::string>{});
}

TEST_F(CompiledPack, IsTheClassicPackAloneWhenNoRemasterIsEmbedded) {
  auto classic{ OpenEmbeddedPack(CLASSIC_PACK).value_or(assets::Fail(assets::PackFault::MISSING_FILE, "no pack compiled in")) };
  ASSERT_TRUE(classic) << assets::Describe(classic.error());
  auto const game{ assets::OpenLayered(std::make_unique<assets::Pack const>(*std::move(classic)), EmbeddedPackFiles(REMASTER_PACK)) };
  ASSERT_TRUE(game) << assets::Describe(game.error());
  EXPECT_EQ((*game)->Ids().size(), 3'262U);
  EXPECT_EQ((*game)->Load("portrait/118").value().texture.Width(), 64U);
}

}
