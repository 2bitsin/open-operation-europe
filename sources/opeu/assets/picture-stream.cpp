#include "opeu/assets/picture-stream.hpp"

#include <algorithm>
#include <array>
#include <ranges>
#include <utility>
#include <vector>

namespace opeu::assets::detail::picture_stream {

namespace {

constexpr auto LINE_UNITS{ std::size_t{ 160 } }; // a 640-pixel screen line, four pixels to a unit
constexpr auto TOP{ 4 * LINE_UNITS };            // blank lines above: a row-up copy reaches four rows back
constexpr auto MAX_WIDTH{ 640 };
constexpr auto MAX_HEIGHT{ 400 };

// Which two planes a sixteen-colour literal byte fills, high nibble first.
constexpr auto PLANE_PAIRS{ std::array<std::array<unsigned, 2>, 6>{ { { 1, 0 }, { 2, 0 }, { 3, 0 }, { 2, 1 }, { 3, 1 }, { 3, 2 } } } };

// Draws one stream into a screen-wide unit buffer as the game's decoder does, then reads the picture back.
class StreamDecoder {
public:
  StreamDecoder(Bytes const bytes, Colours const colours) : _reader{ bytes }, _colours{ colours } { }

  auto Run() -> Decoded<Picture> {
    _width  = _reader.NextWord();
    _height = _reader.NextWord();
    if (_reader.Overrun()) return std::unexpected{ DecodeError::TRUNCATED };
    if (_width < 1 || _width > MAX_WIDTH || _height < 1 || _height > MAX_HEIGHT) {
      return std::unexpected{ DecodeError::BAD_HEADER };
    }
    if (_colours == Colours::SIXTEEN) std::ranges::generate(_runs, [this] { return _reader.NextWord(); });
    _units.assign(TOP + (_height + 1uz) * LINE_UNITS + 16, 0);
    for (auto const row : Indices(_height)) {
      if (!DecodeRow(TOP + row * LINE_UNITS)) return std::unexpected{ DecodeError::TRUNCATED };
    }
    return Picture{ .image = Pixels(), .length = _reader.Position() };
  }

private:
  // Commands until the row's units are covered (the last may run past its end) or the stream runs out.
  auto DecodeRow(std::size_t const start) -> bool {
    auto const end{ start + (std::size_t{ _width } + 3) / 4 };
    auto       at{ start };
    auto const commands{ std::views::iota(0uz) | std::views::take_while([&](std::size_t) { return at < end && !_reader.Overrun(); }) };
    std::ranges::for_each(commands, [&](std::size_t) {
      auto const command{ _reader.Next() };
      at += (command & 0x80) != 0 ? Copy(command, at) : Literal(command, at);
    });
    return !_reader.Overrun();
  }

  // 1 s d d c c c c: c + 1 units from d + 1 units back (s = 0) or d + 1 rows up (s = 1), unit by unit, so an
  // overlapping source repeats a pattern.
  auto Copy(std::uint8_t const command, std::size_t const at) -> std::size_t {
    auto const count{ (command & 0x0Fuz) + 1 };
    auto const distance{ ((command >> 4 & 0x03uz) + 1) * ((command & 0x40) != 0 ? LINE_UNITS : 1) };
    for (auto const unit : std::views::iota(at, at + count)) _units[unit] = _units[unit - distance];
    return count;
  }

  auto Literal(std::uint8_t const command, std::size_t const at) -> std::size_t {
    auto const count{ _colours == Colours::EIGHT ? (command >> 4 & 0x07uz) + 1 : (command & 0x07uz) + 1 };
    auto const value{ _colours == Colours::EIGHT ? static_cast<unsigned>((command & 0x0F) << 8 | _reader.Next()) : SixteenColourValue(command) };
    std::ranges::fill_n(_units.begin() + static_cast<std::ptrdiff_t>(at), static_cast<std::ptrdiff_t>(count), value);
    return count;
  }

  // 0 k k k k r r r: kkkk = 0 zero, 1ttt run table entry ttt, 0111 two bytes for all four planes, 0mmm one byte
  // for the planes PLANE_PAIRS[m - 1].
  auto SixteenColourValue(std::uint8_t const command) -> unsigned {
    auto const mode{ static_cast<unsigned>(command >> 3 & 0x0F) };
    if (mode == 0) return 0;
    if ((mode & 0x08) != 0) return _runs[mode & 0x07];
    if (mode == 0x07) {
      auto const high{ _reader.Next() };
      return static_cast<unsigned>(high << 8 | _reader.Next());
    }
    auto const  c{ _reader.Next() };
    auto const& planes{ PLANE_PAIRS.at(mode - 1) };
    auto const  upper{ planes.front() };
    auto const  lower{ planes.back() };
    return static_cast<unsigned>((c >> 4) << (4 * upper) | (c & 0x0F) << (4 * lower));
  }

  // Unit values (nibble p = plane p, bit 3 the leftmost pixel) to palette indices.
  [[nodiscard]] auto Pixels() const -> Image {
    auto pixels{ std::vector<std::uint8_t>{} };
    pixels.reserve(std::size_t{ _width } * _height);
    for (auto const row : Indices(_height)) {
      for (auto const x : Indices(_width)) {
        auto const unit{ _units[TOP + row * LINE_UNITS + x / 4] };
        pixels.push_back(std::ranges::fold_left(Indices(4), std::uint8_t{ 0 },
                                                [&](std::uint8_t const index, auto const plane) { return static_cast<std::uint8_t>(index | (unit >> (4 * plane + 3 - x % 4) & 1) << plane); }));
      }
    }
    return Image{ _width, _height, std::move(pixels) };
  }

  ByteReader              _reader;
  Colours                 _colours;
  unsigned                _width{};
  unsigned                _height{};
  std::array<unsigned, 8> _runs{};
  std::vector<unsigned>   _units;
};

}

auto DecodePicture(Bytes const bytes, Colours const colours) -> Decoded<Picture> {
  return StreamDecoder{ bytes, colours }.Run();
}

}
