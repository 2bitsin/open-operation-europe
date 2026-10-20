#include "opeu/assets/code-table.hpp"

#include "opeu/assets/table-json.hpp"

#include <algorithm>
#include <format>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <utility>

namespace opeu::assets::detail::code_table {

using namespace std::string_literals;
using namespace std::string_view_literals;

namespace {

using namespace table_json;

constexpr auto HEX_DIGITS{ "0123456789ABCDEF"sv };
constexpr auto GLYPH_PREFIX{ "glyph:"sv };
constexpr auto BYTE_PREFIX{ "byte:"sv };
constexpr auto FIRST_LEAD{ std::uint8_t{ 0x80 } };
constexpr auto FIRST_PRINTABLE{ std::uint8_t{ 0x20 } };
constexpr auto LAST_PRINTABLE{ std::uint8_t{ 0x7E } };

auto ByteAt(std::string_view const text, std::size_t const at) -> std::uint8_t {
  return static_cast<std::uint8_t>(text[at]);
}

// The value of "0x" and `width` upper-case hex digits, or none.
auto HexNumber(std::string_view const text, std::size_t const width) -> std::optional<std::uint32_t> {
  if (text.size() != width + 2 || !text.starts_with("0x")) return std::nullopt;
  auto const digits{ text.substr(2) };
  if (std::ranges::any_of(digits, [](char const c) { return !HEX_DIGITS.contains(c); })) return std::nullopt;
  return std::ranges::fold_left(digits, std::uint32_t{}, [](std::uint32_t const value, char const c) { return (value << 4) | static_cast<std::uint32_t>(HEX_DIGITS.find(c)); });
}

auto TableHex(Json const& record, std::string_view const key, std::size_t const width) -> std::uint32_t {
  auto const text{ record.at(std::string{ key }).get<std::string>() };
  auto const value{ HexNumber(text, width) };
  if (!value) throw std::invalid_argument{ std::format("{} '{}' is not 0x and {} upper-case hex digits", key, text, width) };
  return *value;
}

// How many bytes the UTF-8 sequence led by `lead` takes.
auto SequenceLength(std::uint8_t const lead) -> std::size_t {
  if (lead < 0x80) return 1;
  if ((lead & 0xE0) == 0xC0) return 2;
  if ((lead & 0xF0) == 0xE0) return 3;
  return 4;
}

auto TwoBytes(std::uint32_t const code) -> std::string {
  return std::string{ static_cast<char>(code >> 8 & 0xFF), static_cast<char>(code & 0xFF) };
}

// Every token of `input` in turn, `next` giving each one; the first failure stops the scan.
template <class _Next>
auto Scan(std::string_view const input, _Next next) -> Packed<std::string> {
  auto       text{ std::string{} };
  auto       at{ 0uz };
  auto       failure{ std::optional<PackError>{} };
  auto const tokens{ std::views::iota(0uz) | std::views::take_while([&](std::size_t) { return at < input.size() && !failure; }) };
  std::ranges::for_each(tokens, [&](std::size_t) {
    auto const token{ next(input.substr(at)) };
    if (!token) {
      failure = token.error();
      return;
    }
    text += token->text;
    at   += token->length;
  });
  if (failure) return std::unexpected{ *failure };
  return text;
}

auto NamedBytes(Json const& table) -> std::vector<NamedByte> {
  return table.at("named_bytes")
         | std::views::transform([](Json const& record) { return NamedByte{ .byte = static_cast<std::uint8_t>(TableHex(record, "byte", 2)), .name = record.at("name").get<std::string>() }; })
         | std::ranges::to<std::vector>();
}

auto Characters(Json const& table) -> std::vector<CodedCharacter> {
  return table.at("characters") | std::views::transform([](Json const& record) {
           auto character{ record.at("char").get<std::string>() };
           if (character.empty() || SequenceLength(ByteAt(character, 0)) != character.size() || IsAscii(character))
             throw std::invalid_argument{ std::format("'{}' is not one Chinese character", character) };
           return CodedCharacter{ .code = TableHex(record, "code", 4), .character = std::move(character) };
         })
         | std::ranges::to<std::vector>();
}

// Whether no value of a projection of the characters appears twice.
template <class _Projection>
auto AllDistinct(std::vector<CodedCharacter> const& characters, _Projection projection) -> bool {
  auto values{ characters | std::views::transform(projection) | std::ranges::to<std::vector>() };
  std::ranges::sort(values);
  return std::ranges::adjacent_find(values) == values.end();
}

auto CodeTableOf(Json const& table) -> CodeTable {
  auto const characters{ Characters(table) };
  if (!AllDistinct(characters, &CodedCharacter::code) || !AllDistinct(characters, &CodedCharacter::character)) throw std::invalid_argument{ "a code or a character appears twice" };
  return CodeTable{ NamedBytes(table), characters };
}

}

CodeTable::CodeTable(std::vector<NamedByte> const& names, std::vector<CodedCharacter> const& characters) {
  std::ranges::for_each(names, [this](NamedByte const& name) {
    _names.emplace(name.byte, name.name);
    _named_bytes.emplace(name.name, name.byte);
  });
  std::ranges::for_each(characters, [this](CodedCharacter const& character) {
    _characters.emplace(character.code, character.character);
    _codes.emplace(character.character, character.code);
  });
}

auto CodeTable::Decode(std::string_view const bytes) const -> std::string {
  return Scan(bytes, [this](std::string_view const rest) -> Packed<Token> { return DecodedToken(rest); }).value_or(std::string{});
}

auto CodeTable::Encode(std::string_view const message) const -> Packed<std::string> {
  return Scan(message, [this](std::string_view const rest) { return EncodedToken(rest); });
}

auto CodeTable::DecodedToken(std::string_view const rest) const -> Token {
  auto const lead{ ByteAt(rest, 0) };
  if (auto const name{ _names.find(lead) }; name != _names.end()) return Token{ .text = std::format("{{{}}}", name->second), .length = 1 };
  if (lead >= FIRST_PRINTABLE && lead <= LAST_PRINTABLE) return Token{ .text = std::string(1, rest.front()), .length = 1 };
  if (lead < FIRST_LEAD || rest.size() < 2) return Token{ .text = std::format("{{{}0x{:02X}}}", BYTE_PREFIX, lead), .length = 1 };
  auto const code{ std::uint32_t{ lead } << 8 | ByteAt(rest, 1) };
  auto const character{ _characters.find(code) };
  return Token{ .text = character != _characters.end() ? character->second : std::format("{{{}0x{:04X}}}", GLYPH_PREFIX, code), .length = 2 };
}

auto CodeTable::EncodedToken(std::string_view const rest) const -> Packed<Token> {
  auto const lead{ ByteAt(rest, 0) };
  if (rest.front() == '{') return EncodedName(rest);
  if (lead >= FIRST_PRINTABLE && lead <= LAST_PRINTABLE) return Token{ .text = std::string(1, rest.front()), .length = 1 };
  return EncodedCharacter(rest);
}

// {name}, {glyph:0xXXXX} or {byte:0xXX}.
auto CodeTable::EncodedName(std::string_view const rest) const -> Packed<Token> {
  auto const close{ rest.find('}') };
  if (close == std::string_view::npos) return Fail(PackFault::BAD_TEXT, std::format("unclosed name '{}'", rest.substr(0, 16)));
  auto const inside{ rest.substr(1, close - 1) };
  auto const length{ close + 1 };
  if (auto const named{ _named_bytes.find(inside) }; named != _named_bytes.end()) return Token{ .text = std::string(1, static_cast<char>(named->second)), .length = length };
  if (inside.starts_with(GLYPH_PREFIX)) {
    if (auto const code{ HexNumber(inside.substr(GLYPH_PREFIX.size()), 4) }; code && *code >> 8 >= FIRST_LEAD) return Token{ .text = TwoBytes(*code), .length = length };
  }
  if (inside.starts_with(BYTE_PREFIX)) {
    if (auto const byte{ HexNumber(inside.substr(BYTE_PREFIX.size()), 2) }) return Token{ .text = std::string(1, static_cast<char>(*byte)), .length = length };
  }
  return Fail(PackFault::BAD_TEXT, std::format("no byte is named '{}'", inside));
}

auto CodeTable::EncodedCharacter(std::string_view const rest) const -> Packed<Token> {
  auto const length{ std::min(SequenceLength(ByteAt(rest, 0)), rest.size()) };
  auto const character{ rest.substr(0, length) };
  auto const code{ _codes.find(character) };
  if (code == _codes.end()) return Fail(PackFault::BAD_TEXT, std::format("no code for '{}'", IsAscii(character) ? std::format("byte 0x{:02X}", ByteAt(character, 0)) : std::string{ character }));
  return Token{ .text = TwoBytes(code->second), .length = length };
}

auto ReadCodeTable(Bytes const bytes) -> Packed<CodeTable> {
  return ReadTable(bytes, "code_table", CodeTableOf);
}

}
