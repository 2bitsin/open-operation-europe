#include "opeu/intro/segment-bytes.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <ranges>
#include <vector>

namespace opeu::intro::detail::segment_bytes_test {
namespace {

using Bytes = std::array<std::uint8_t, SEGMENT_SIZE>;

// A segment whose byte at each offset is one more than the offset modulo 255, so no byte is zero.
auto Numbered() -> std::unique_ptr<Bytes> {
  auto bytes{ std::make_unique<Bytes>() };
  std::ranges::transform(std::views::iota(0uz, SEGMENT_SIZE), bytes->begin(), [](std::size_t at) { return static_cast<std::uint8_t>(at % 255 + 1); });
  return bytes;
}

auto Write(Bytes& bytes, std::uint16_t at, std::initializer_list<std::uint8_t> values) -> void {
  std::ranges::for_each(values, [&](std::uint8_t value) { bytes[at++] = value; });
}

auto Read(Bytes const& bytes, std::uint16_t at, std::uint16_t count) -> std::vector<std::uint8_t> {
  return std::views::iota(std::uint16_t{ 0 }, count) | std::views::transform([&](std::uint16_t step) { return bytes[static_cast<std::uint16_t>(at + step)]; }) | std::ranges::to<std::vector>();
}

TEST(CopyBytes, MovesABlockBetweenDisjointPlaces) {
  auto const bytes{ Numbered() };
  Write(*bytes, 0x0100, { 1, 2, 3, 4 });
  CopyBytes(Segment{ *bytes }, { .source = 0x0100, .destination = 0x0200, .size = 4 });
  EXPECT_EQ(Read(*bytes, 0x0200, 4), (std::vector<std::uint8_t>{ 1, 2, 3, 4 }));
}

TEST(CopyBytes, KeepsAnOverlappingBlockMovedDownIntact) {
  auto const bytes{ Numbered() };
  Write(*bytes, 0x0102, { 1, 2, 3, 4 });
  CopyBytes(Segment{ *bytes }, { .source = 0x0102, .destination = 0x0100, .size = 4 });
  EXPECT_EQ(Read(*bytes, 0x0100, 4), (std::vector<std::uint8_t>{ 1, 2, 3, 4 }));
}

TEST(CopyBytes, KeepsAnOverlappingBlockMovedUpIntact) {
  auto const bytes{ Numbered() };
  Write(*bytes, 0x0100, { 1, 2, 3, 4 });
  CopyBytes(Segment{ *bytes }, { .source = 0x0100, .destination = 0x0102, .size = 4 });
  EXPECT_EQ(Read(*bytes, 0x0102, 4), (std::vector<std::uint8_t>{ 1, 2, 3, 4 }));
}

TEST(CopyBytes, WrapsPastTheEndOfTheSegment) {
  auto const bytes{ Numbered() };
  Write(*bytes, 0xfffe, { 1, 2, 3, 4 });
  CopyBytes(Segment{ *bytes }, { .source = 0xfffe, .destination = 0x1000, .size = 4 });
  EXPECT_EQ(Read(*bytes, 0x1000, 4), (std::vector<std::uint8_t>{ 1, 2, 3, 4 }));
}

TEST(CopyBytes, CopiesNothingForAnEmptyBlock) {
  auto const bytes{ Numbered() };
  auto const before{ *bytes };
  CopyBytes(Segment{ *bytes }, { .source = 0x0100, .destination = 0x0200, .size = 0 });
  EXPECT_EQ(*bytes, before);
}

TEST(StringLength, CountsTheBytesBeforeTheTerminator) {
  auto const bytes{ Numbered() };
  Write(*bytes, 0x0100, { 'a', 'b', 'c', 0 });
  EXPECT_EQ(StringLength(ConstSegment{ *bytes }, 0x0100), 3);
}

TEST(StringLength, IsZeroForAnEmptyString) {
  auto const bytes{ Numbered() };
  Write(*bytes, 0x0100, { 0 });
  EXPECT_EQ(StringLength(ConstSegment{ *bytes }, 0x0100), 0);
}

TEST(StringLength, WrapsPastTheEndOfTheSegment) {
  auto const bytes{ Numbered() };
  Write(*bytes, 0xfffe, { 'a', 'b', 'c', 0 });
  EXPECT_EQ(StringLength(ConstSegment{ *bytes }, 0xfffe), 3);
}

TEST(AsciiUpper, RaisesLowerCaseLettersOnly) {
  EXPECT_EQ(AsciiUpper('l'), 'L');
  EXPECT_EQ(AsciiUpper('L'), 'L');
  EXPECT_EQ(AsciiUpper('%'), '%');
  EXPECT_EQ(AsciiUpper(0xe1), 0xe1);
}

} // namespace
} // namespace opeu::intro::detail::segment_bytes_test
