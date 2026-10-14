#pragma once
// What the tables' JSON writers and readers share (tables/*.json beside the classic pack, docs/asset-pack.md): the
// format, grids written as rows, numbers read back with their range checked, and game text, which is ASCII except
// where it is in KOEI's double-byte code.

#include "opeu/assets/bytes.hpp"
#include "opeu/assets/pack-error.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace opeu::assets::detail::table_json {

using Json = nlohmann::json;

inline constexpr auto TABLE_FORMAT{ std::string_view{ "opeu-tables/1" } };

// A table file: the format and one member `name` holding the table, as compact JSON text.
[[nodiscard]] inline auto TableText(std::string_view const name, Json value) -> std::string {
  auto table                 = Json::object(); // not braces: a braced json is an array holding it
  table["format"]            = TABLE_FORMAT;
  table[std::string{ name }] = std::move(value);
  return table.dump();
}

// The table `name` of a table file's bytes read by `read` (which throws on a malformed record), or why it cannot be.
template <class _Read>
[[nodiscard]] auto ReadTable(Bytes const bytes, std::string_view const name, _Read read) -> Packed<decltype(read(Json{}))> {
  try {
    auto const table = Json::parse(AsText(bytes)); // not braces, as above
    if (table.at("format").get<std::string>() != TABLE_FORMAT) return Fail(PackFault::BAD_TABLE, std::format("{}: not {}", name, TABLE_FORMAT));
    return read(table.at(std::string{ name }));
  } catch (std::exception const& error) {
    return Fail(PackFault::BAD_TABLE, std::format("{}: {}", name, error.what()));
  }
}

// The number `key` of a record, refused above `limit`.
[[nodiscard]] inline auto Number(Json const& record, std::string_view const key, std::uint32_t const limit) -> std::uint32_t {
  auto const value{ record.at(std::string{ key }).get<std::uint32_t>() };
  if (value > limit) throw std::out_of_range{ std::format("{} {} above {}", key, value, limit) };
  return value;
}

[[nodiscard]] inline auto ByteField(Json const& record, std::string_view const key) -> std::uint8_t {
  return static_cast<std::uint8_t>(Number(record, key, 0xFF));
}

[[nodiscard]] inline auto WordField(Json const& record, std::string_view const key) -> std::uint32_t {
  return Number(record, key, 0xFFFF);
}

// Refuses anything but an array of `count` entries; `what` names it in the error.
inline auto CheckCount(Json const& entries, std::string_view const what, std::size_t const count) -> void {
  if (!entries.is_array() || entries.size() != count) throw std::length_error{ std::format("{} holds {} entries, not {}", what, entries.size(), count) };
}

// The array `key` of a record, refused unless it holds `count` entries.
[[nodiscard]] inline auto Sized(Json const& record, std::string_view const key, std::size_t const count) -> Json const& {
  auto const& entries{ record.at(std::string{ key }) };
  CheckCount(entries, key, count);
  return entries;
}

// `count` numbers of the array `key`, each at most `limit`.
template <std::size_t COUNT>
[[nodiscard]] auto Numbers(Json const& record, std::string_view const key, std::uint32_t const limit) -> std::array<std::uint32_t, COUNT> {
  auto numbers{ std::array<std::uint32_t, COUNT>{} };
  std::ranges::transform(Sized(record, key, COUNT), numbers.begin(), [key, limit](Json const& value) {
    auto const number{ value.get<std::uint32_t>() };
    if (number > limit) throw std::out_of_range{ std::format("{} holds {} above {}", key, number, limit) };
    return number;
  });
  return numbers;
}

// Cells as rows of `side`.
template <std::ranges::forward_range _Cells>
[[nodiscard]] auto Rows(_Cells const& cells, std::size_t const side) -> Json {
  auto const values{ cells | std::views::transform([](auto const cell) { return static_cast<std::uint32_t>(cell); }) | std::ranges::to<std::vector<std::uint32_t>>() };
  auto const all{ std::span{ values } };
  return Json(Indices(values.size() / side) | std::views::transform([all, side](std::size_t const row) { return Json(all.subspan(row * side, side) | std::ranges::to<std::vector<std::uint32_t>>()); })
              | std::ranges::to<std::vector<Json>>());
}

// A square grid of SIDE x SIDE cells, row-major.
template <std::size_t SIDE>
using SquareCells = std::array<std::uint8_t, SIDE * SIDE>;

// The cells of the rows `key` (SIDE rows of SIDE), each at most `limit`.
template <std::size_t SIDE>
[[nodiscard]] auto Cells(Json const& record, std::string_view const key, std::uint32_t const limit) -> SquareCells<SIDE> {
  auto        cells{ SquareCells<SIDE>{} };
  auto const& rows{ Sized(record, key, SIDE) };
  auto const  values{ rows | std::views::transform([key](Json const& row) {
                       CheckCount(row, key, SIDE);
                       return row.template get<std::vector<std::uint32_t>>();
                     })
                     | std::views::join | std::ranges::to<std::vector<std::uint32_t>>() };
  std::ranges::transform(values, cells.begin(), [key, limit](std::uint32_t const value) {
    if (value > limit) throw std::out_of_range{ std::format("{} holds {} above {}", key, value, limit) };
    return static_cast<std::uint8_t>(value);
  });
  return cells;
}

// Whether every byte of a text is ASCII.
[[nodiscard]] inline auto IsAscii(std::string_view const text) -> bool {
  return std::ranges::all_of(text, [](char const c) { return static_cast<std::uint8_t>(c) < 0x80; });
}

// A game text as a record member: under `name` when ASCII; else, in KOEI's double-byte code, its bytes in hex under
// `name`_koei (the code has no Unicode table yet).
[[nodiscard]] inline auto TextMember(std::string_view const name, std::string_view const text) -> Json {
  auto member = Json::object();
  if (IsAscii(text)) {
    member[std::string{ name }] = text;
    return member;
  }
  member[std::format("{}_koei", name)] = text | std::views::transform([](char const c) { return std::format("{:02x}", static_cast<std::uint8_t>(c)); }) | std::views::join
                                         | std::ranges::to<std::string>();
  return member;
}

// The value of a hex digit.
[[nodiscard]] inline auto HexDigit(char const digit) -> std::uint32_t {
  constexpr auto DIGITS{ std::string_view{ "0123456789abcdef" } };
  auto const     found{ DIGITS.find(digit) };
  if (found == std::string_view::npos) throw std::invalid_argument{ std::format("'{}' is no hex digit", digit) };
  return static_cast<std::uint32_t>(found);
}

// The game text a record member holds, as TextMember writes it.
[[nodiscard]] inline auto TextOf(Json const& record, std::string_view const name) -> std::string {
  if (record.contains(name)) {
    auto text{ record.at(std::string{ name }).get<std::string>() };
    if (!IsAscii(text)) throw std::invalid_argument{ std::format("{} is not ASCII", name) };
    return text;
  }
  auto const hex{ record.at(std::format("{}_koei", name)).get<std::string>() };
  if (hex.size() % 2 != 0) throw std::invalid_argument{ std::format("{}_koei has an odd number of digits", name) };
  return Indices(hex.size() / 2) | std::views::transform([&hex](std::size_t const n) { return static_cast<char>((HexDigit(hex.at(2 * n)) << 4) | HexDigit(hex.at((2 * n) + 1))); })
         | std::ranges::to<std::string>();
}

}
