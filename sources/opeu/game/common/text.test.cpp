#include "opeu/game/common/text.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <memory>
#include <string_view>

namespace opeu::game::detail::text_test {
namespace {
constexpr std::uint16_t NAME_AT{ 0xf000 };
constexpr std::uint16_t FORMAT_AT{ 0xf100 };

// A zeroed data segment holding a name at NAME_AT.
auto Segment() -> std::unique_ptr<DataSegment> {
  auto data{ std::make_unique<DataSegment>() };
  StoreText(*data, NAME_AT, "Rommel");
  return data;
}

auto Format(std::string_view format, std::initializer_list<std::uint16_t> words) -> std::string {
  auto const data{ Segment() };
  return FormatText(*data, format, std::span{ words.begin(), words.size() });
}
} // namespace

TEST(CString, EndsAtTheTerminator) {
  auto const data{ Segment() };
  EXPECT_EQ(CString(*data, NAME_AT), "Rommel");
  EXPECT_EQ(CString(std::array{ 'a', 'b', '\0', 'c' }), "ab");
}

TEST(StoreText, WritesTheTextWithItsTerminator) {
  auto data{ Segment() };
  StoreText(*data, FORMAT_AT, "%d");
  EXPECT_EQ(CString(*data, FORMAT_AT), "%d");
}

TEST(FormatText, FormatsSignedUnsignedAndHexWords) {
  EXPECT_EQ(Format("%d %u %x %o", { 0xffff, 0xffff, 0xbeef, 8 }), "-1 65535 BEEF 10");
  EXPECT_EQ(Format("%+d", { 5 }), "+5");
}

TEST(FormatText, PadsToTheWidthOnEitherSide) {
  EXPECT_EQ(Format("[%5d][%-5d][%05d]", { 42, 42, 42 }), "[   42][42   ][00042]");
}

TEST(FormatText, CountsTheSignAgainstTheWidth) {
  EXPECT_EQ(Format("[%4d]", { 0xfffe }), "[  -2]");
}

TEST(FormatText, TakesStringsByNearPointerAndCutsThemAtThePrecision) {
  EXPECT_EQ(Format("%s/%.3s/%8s", { NAME_AT, NAME_AT, NAME_AT }), "Rommel/Rom/  Rommel");
}

TEST(FormatText, TakesALongFromTwoWords) {
  EXPECT_EQ(Format("%ld", { 0x86a0, 0x0001 }), "100000");
  EXPECT_EQ(Format("%lD", { 0xffff, 0xffff }), "-1");
}

TEST(FormatText, CopiesAnUnknownLetterAndStopsAtAFinalPercent) {
  EXPECT_EQ(Format("100%% %c", { 'x' }), "100% x");
  EXPECT_EQ(Format("done%", {}), "done");
}

TEST(FormatText, ReadsMissingWordsAsZero) {
  EXPECT_EQ(Format("%d-%d", { 7 }), "7-0");
}
} // namespace opeu::game::detail::text_test
