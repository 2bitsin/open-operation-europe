#pragma once
// KOEI's text code and Unicode (tools/assets/code-table.json, copied into the pack as texts/code-table.json;
// docs/asset-pack.md, "Texts"): ASCII bytes stand for themselves, a byte from 0x80 up leads a two-byte code of a
// Chinese character, and a few bytes are written by name. A text in the code becomes a UTF-8 message and back, byte
// for byte: a code the table lacks is written {glyph:0xXXXX}, a byte no rule covers {byte:0xXX}.

#include "opeu/assets/bytes.hpp"
#include "opeu/assets/pack-error.hpp"

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace opeu::assets::detail::code_table {

inline constexpr auto CODE_TABLE{ std::string_view{ "texts/code-table.json" } };

// A byte written as {name} in a message.
struct NamedByte {
  std::uint8_t byte{};
  std::string  name;
};

// A two-byte code and the character it stands for, as UTF-8.
struct CodedCharacter {
  std::uint32_t code{};
  std::string   character;
};

class CodeTable {
public:
  CodeTable(std::vector<NamedByte> const& names, std::vector<CodedCharacter> const& characters);

  // The UTF-8 message of a text in KOEI's code.
  [[nodiscard]] auto Decode(std::string_view bytes) const -> std::string;
  // The text in KOEI's code of a UTF-8 message, or BAD_TEXT naming what has no code.
  [[nodiscard]] auto Encode(std::string_view message) const -> Packed<std::string>;

private:
  // What the front of the input becomes, and how many of its bytes that takes.
  struct Token {
    std::string text;
    std::size_t length{};
  };

  [[nodiscard]] auto DecodedToken(std::string_view rest) const -> Token;
  [[nodiscard]] auto EncodedToken(std::string_view rest) const -> Packed<Token>;
  [[nodiscard]] auto EncodedName(std::string_view rest) const -> Packed<Token>;
  [[nodiscard]] auto EncodedCharacter(std::string_view rest) const -> Packed<Token>;

  std::map<std::uint8_t, std::string>               _names;
  std::map<std::string, std::uint8_t, std::less<>>  _named_bytes;
  std::map<std::uint32_t, std::string>              _characters;
  std::map<std::string, std::uint32_t, std::less<>> _codes;
};

[[nodiscard]] auto ReadCodeTable(Bytes bytes) -> Packed<CodeTable>;

}

namespace opeu::assets {
using detail::code_table::CODE_TABLE;
using detail::code_table::CodedCharacter;
using detail::code_table::CodeTable;
using detail::code_table::NamedByte;
using detail::code_table::ReadCodeTable;
}
