// The game's texts as UTF-8 (packs.embed/classic/texts/): every text of the shipped files has its message, each message
// is what the code table decodes and turns back into the very bytes the file holds, no glyph resists, and the code
// table keeps what it cannot name and refuses what has no code.
#include "packs.hpp"
#include "shipped.hpp"

#include "opeu/assets/code-table.hpp"
#include "opeu/assets/glyphs.hpp"
#include "opeu/assets/messages.hpp"
#include "opeu/assets/scenario.hpp"

#include <gtest/gtest.h>

#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace opeu::assets {

namespace {

using test::ClassicFile;
using test::Shipped;
using test::ToBytes;

auto ShippedTexts() -> Messages {
  return ScenarioTexts(DecodeScenarioFile(Shipped("SCEDATA")).value());
}

auto PackedCodeTable() -> CodeTable {
  return ReadCodeTable(ClassicFile(CODE_TABLE)).value();
}

auto PackedMessages() -> Messages {
  return ReadMessages(ClassicFile(MESSAGES)).value();
}

auto Ids(Messages const& messages) -> std::vector<std::string> {
  return messages | std::views::keys | std::ranges::to<std::vector>();
}

auto CodeBytes(std::uint32_t const code) -> std::string {
  return std::string{ static_cast<char>(code >> 8), static_cast<char>(code & 0xFF) };
}

}

TEST(Messages, HoldEveryTextOfTheShippedFiles) {
  EXPECT_EQ(Ids(PackedMessages()), Ids(ShippedTexts()));
}

TEST(Messages, AreWhatTheCodeTableDecodes) {
  EXPECT_EQ(PackedMessages(), MessagesOf(ShippedTexts(), PackedCodeTable()));
}

TEST(Messages, EncodeToTheBytesTheShippedFileHolds) {
  auto const table{ PackedCodeTable() };
  auto const messages{ PackedMessages() };
  auto const texts{ ShippedTexts() };
  auto const differing{ texts | std::views::filter([&](auto const& text) { return table.Encode(messages.at(text.first)) != text.second; }) | std::views::keys | std::ranges::to<std::vector>() };
  EXPECT_EQ(differing, std::vector<std::string>{});
  EXPECT_EQ(texts.size(), 290U); // 6 descriptions, 12 conditions, 272 names
}

TEST(Messages, LeaveNoGlyphUnnamed) {
  auto const resisting{ PackedMessages() | std::views::filter([](auto const& message) { return message.second.contains("{glyph:") || message.second.contains("{byte:"); }) | std::views::keys
                        | std::ranges::to<std::vector>() };
  EXPECT_EQ(resisting, std::vector<std::string>{});
}

TEST(Messages, NameTheLineBreaks) {
  auto const description{ PackedMessages().at("scenario/0/description") };
  EXPECT_TRUE(description.contains("{newline}"));
  EXPECT_FALSE(description.contains('\n'));
}

TEST(Messages, SpellSetupZerosNamesInChinese) {
  auto const messages{ PackedMessages() };
  EXPECT_EQ(messages.at("setup/0/person/00/name"), "龍施泰特");
  EXPECT_EQ(messages.at("setup/0/objective/09/name"), "色當");
  EXPECT_EQ(messages.at("setup/1/person/00/name"), "Rundstedt");
}

TEST(Messages, ReadBackWhatTheyWrite) {
  auto const messages{ PackedMessages() };
  EXPECT_EQ(ReadMessages(ToBytes(MessagesText(messages))), messages);
}

TEST(CodeTable, GivesEveryGlyphOfTheEndingFontACharacter) {
  auto const table{ PackedCodeTable() };
  auto const unnamed{ EndingGlyphCodes() | std::views::filter([&table](std::uint32_t const code) { return table.Decode(CodeBytes(code)).starts_with('{'); }) | std::ranges::to<std::vector>() };
  EXPECT_EQ(unnamed, std::vector<std::uint32_t>{});
}

TEST(CodeTable, KeepsWhatItCannotName) {
  auto const table{ PackedCodeTable() };
  auto const bytes{ std::string{ "\x81\x40\x05{" } };
  EXPECT_EQ(table.Decode(bytes), "{glyph:0x8140}{byte:0x05}{brace}");
  EXPECT_EQ(table.Encode(table.Decode(bytes)), bytes);
}

TEST(CodeTable, RefusesWhatHasNoCode) {
  auto const table{ PackedCodeTable() };
  EXPECT_EQ(table.Encode("\xE2\x82\xAC").error().fault, PackFault::BAD_TEXT); // the euro sign
  EXPECT_EQ(table.Encode("{bogus}").error().fault, PackFault::BAD_TEXT);
  EXPECT_EQ(table.Encode("{newline").error().fault, PackFault::BAD_TEXT);
  EXPECT_EQ(table.Encode("\n").error().fault, PackFault::BAD_TEXT);
  EXPECT_EQ(table.Encode("{glyph:0x0041}").error().fault, PackFault::BAD_TEXT);
}

TEST(CodeTable, RefusesACodeTwice) {
  auto const text{ std::string_view{ R"({"format": "opeu-tables/1", "code_table": {"named_bytes": [], "characters": [{"code": "0x92A0", "char": "一"}, {"code": "0x92A0", "char": "了"}]}})" } };
  EXPECT_EQ(ReadCodeTable(ToBytes(text)).error().fault, PackFault::BAD_TABLE);
}

TEST(CodeTable, RefusesACharacterThatIsNoChineseCharacter) {
  auto const text{ std::string_view{ R"({"format": "opeu-tables/1", "code_table": {"named_bytes": [], "characters": [{"code": "0x92A0", "char": "ab"}]}})" } };
  EXPECT_EQ(ReadCodeTable(ToBytes(text)).error().fault, PackFault::BAD_TABLE);
}

}
