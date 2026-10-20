// The FM banks as JSON (packs.embed/classic/music/): each table reads back into a bank whose bytes are the bank the
// recovered program holds (its size and fingerprint; the extract-assets probe compares the bytes and the FM driver's
// register stream themselves), decodes back into the same parts, and a malformed table or bank is refused. The
// tables are the checked-in pack's: a clone without it skips.
#include "packs.hpp"

#include "opeu/assets/fingerprint.hpp"
#include "opeu/assets/music-bank.hpp"
#include "opeu/assets/music-table.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace opeu::assets {

namespace {

using test::ClassicFile;
using test::ToBytes;

using MusicTables = test::CheckedInClassicTest;
using MusicBanks  = test::CheckedInClassicTest;

// What a table's bank is: the bank's size and FNV-1a fingerprint, and how many parts of each kind it holds.
struct Expected {
  std::size_t   bytes;
  std::uint64_t fingerprint;
  std::size_t   instruments;
  std::size_t   drum_kits;
  std::size_t   tracks;
};

// In the order of MUSIC_TABLES: MAIN's song bank, MAIN's effect bank (which follows it in MAIN's data), the opening's song
// bank.
constexpr auto EXPECTED{
  std::to_array<Expected>({
                           { .bytes = 31674, .fingerprint = 0x97CE5E191DC9E1B0, .instruments = 51, .drum_kits = 13, .tracks = 89 },
                           { .bytes = 847, .fingerprint = 0xA7F224E57939286A, .instruments = 18, .drum_kits = 0, .tracks = 23 },
                           { .bytes = 8856, .fingerprint = 0x612F7191CFE70459, .instruments = 15, .drum_kits = 3, .tracks = 15 },
                           }
  )
};

auto PackedBank(std::size_t const n) -> MusicBank {
  return ReadMusicBank(ClassicFile(MUSIC_TABLES.at(n).path)).value();
}

auto FingerprintOf(std::vector<std::byte> const& bytes) -> std::uint64_t {
  auto fingerprint{ Fingerprint{} };
  fingerprint.Add(bytes);
  return fingerprint.Value();
}

// A table text with one piece replaced.
auto Edited(std::size_t const n, std::string_view const from, std::string_view const to) -> std::vector<std::byte> {
  auto       text{ MusicBankText(PackedBank(n)) };
  auto const at{ text.find(from) };
  EXPECT_NE(at, std::string::npos) << from;
  if (at != std::string::npos) text.replace(at, from.size(), to);
  return ToBytes(text);
}

}

TEST_F(MusicTables, EncodeIntoTheBanksTheProgramsHold) {
  std::ranges::for_each(std::views::zip(Indices(MUSIC_TABLES.size()), EXPECTED), [](auto const n_expected) {
    auto const [n, expected]{ n_expected };
    auto const bytes{ EncodeMusicBank(PackedBank(n)) };
    EXPECT_EQ(bytes.size(), expected.bytes) << MUSIC_TABLES.at(n).path;
    EXPECT_EQ(FingerprintOf(bytes), expected.fingerprint) << MUSIC_TABLES.at(n).path;
  });
}

TEST_F(MusicTables, HoldTheBanksParts) {
  std::ranges::for_each(std::views::zip(Indices(MUSIC_TABLES.size()), EXPECTED), [](auto const n_expected) {
    auto const [n, expected]{ n_expected };
    auto const bank{ PackedBank(n) };
    EXPECT_EQ(bank.entries.size(), MUSIC_TABLES.at(n).layout.entries);
    EXPECT_EQ(bank.instruments.size(), expected.instruments);
    EXPECT_EQ(bank.drum_kits.size(), expected.drum_kits);
    EXPECT_EQ(bank.tracks.size(), expected.tracks);
  });
}

TEST_F(MusicTables, DecodeBackIntoTheSameParts) {
  std::ranges::for_each(Indices(MUSIC_TABLES.size()), [](std::size_t const n) {
    auto const bank{ PackedBank(n) };
    EXPECT_EQ(DecodeMusicBank(EncodeMusicBank(bank), MUSIC_TABLES.at(n).layout), bank) << MUSIC_TABLES.at(n).path;
  });
}

TEST_F(MusicTables, ReadBackWhatTheyWrite) {
  auto const bank{ PackedBank(0) };
  EXPECT_EQ(ReadMusicBank(ToBytes(MusicBankText(bank))), bank);
}

TEST_F(MusicTables, NameNotesInstrumentsAndDrums) {
  auto const bank{ PackedBank(0) };
  auto const text{ MusicBankText(bank) };
  EXPECT_TRUE(text.contains(R"({"op":"instrument","name":"instrument-00"})"));
  EXPECT_TRUE(text.contains(R"({"op":"note","note":"C3","length":12,"gate":11})"));
  EXPECT_TRUE(text.contains(R"("op":"drums","drums":[)"));
  EXPECT_FALSE(text.contains(R"("op":"ignored")"));
}

TEST_F(MusicTables, RefuseAnUnknownInstrument) {
  EXPECT_EQ(ReadMusicBank(Edited(0, R"("name":"instrument-00"})", R"("name":"instrument-99"})")).error().fault, PackFault::BAD_TABLE);
}

TEST_F(MusicTables, RefuseANoteAboveTheTop) {
  EXPECT_EQ(ReadMusicBank(Edited(0, R"("note":"C3","length")", R"("note":"C8","length")")).error().fault, PackFault::BAD_TABLE);
}

TEST_F(MusicTables, RefuseARhythmSlotPlayingAMelody) {
  EXPECT_EQ(ReadMusicBank(Edited(0, R"("rhythm":"song-01/rhythm")", R"("rhythm":"song-01/voice-0")")).error().fault, PackFault::BAD_TABLE);
}

TEST_F(MusicTables, RefuseATrackWithoutItsEnd) {
  EXPECT_EQ(ReadMusicBank(Edited(2, R"({"op":"end"}])", "]")).error().fault, PackFault::BAD_TABLE);
}

TEST_F(MusicBanks, RefuseBytesTheirPartsDoNotTile) {
  auto bytes{ EncodeMusicBank(PackedBank(1)) };
  bytes.push_back(std::byte{ 0 });
  EXPECT_EQ(DecodeMusicBank(bytes, MUSIC_TABLES.at(1).layout).error(), DecodeError::BAD_LAYOUT);
}

TEST_F(MusicBanks, RefuseATrackRunningPastTheEnd) {
  auto bytes{ EncodeMusicBank(PackedBank(1)) };
  bytes.pop_back();
  EXPECT_EQ(DecodeMusicBank(bytes, MUSIC_TABLES.at(1).layout).error(), DecodeError::TRUNCATED);
}

}
