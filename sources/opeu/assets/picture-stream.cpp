#include "opeu/assets/picture-stream.hpp"

#include "opeu/assets/fingerprint.hpp"

#include <algorithm>
#include <array>
#include <ranges>
#include <utility>
#include <vector>

namespace opeu::assets::detail::picture_stream {

namespace {

constexpr auto LINE_UNITS{ std::size_t{ 160 } }; // a 640-pixel screen line, four pixels to a unit
constexpr auto TOP{ 4 * LINE_UNITS };            // blank lines above: a row-up copy reaches four rows back
constexpr auto MAX_WIDTH{ 640U };
constexpr auto MAX_HEIGHT{ 400U };

// Which two planes a sixteen-colour literal byte fills, high nibble first.
constexpr auto PLANE_PAIRS{ std::array<std::array<unsigned, 2>, 6>{ { { 1, 0 }, { 2, 0 }, { 3, 0 }, { 2, 1 }, { 3, 1 }, { 3, 2 } } } };

// The bytes of a stream from the caller's reader, counted and fingerprinted as they are read.
class SourceReader {
public:
  explicit SourceReader(NextByte const& next) : _next{ next } { }

  auto Next() -> std::uint8_t {
    auto const byte{ _next.get()() };
    _fingerprint.Add(byte);
    ++_at;
    return byte;
  }
  auto NextWord() -> unsigned {
    auto const low{ Next() };
    return static_cast<unsigned>(low | Next() << 8);
  }
  [[nodiscard]] auto Position() const noexcept -> std::size_t {
    return _at;
  }
  [[nodiscard]] static constexpr auto Overrun() noexcept -> bool {
    return false;
  }
  [[nodiscard]] auto Fingerprinted() const noexcept -> std::uint64_t {
    return _fingerprint.Value();
  }

private:
  std::reference_wrapper<NextByte const> _next;
  std::size_t                            _at{};
  Fingerprint                            _fingerprint;
};

// The value of pixel `x` of a unit (nibble p = plane p, bit 3 the leftmost pixel) as a palette index.
[[nodiscard]] auto IndexOf(unsigned const unit, std::size_t const x) -> std::uint8_t {
  return std::ranges::fold_left(Indices(4), std::uint8_t{ 0 },
                                [&](std::uint8_t const index, auto const plane) { return static_cast<std::uint8_t>(index | (unit >> (4 * plane + 3 - x % 4) & 1) << plane); });
}

// Draws one stream into a screen-wide unit buffer as the game's decoder does, then reads the picture back.
template <class Reader>
class StreamDecoder {
public:
  StreamDecoder(Reader& reader, Colours const colours) : _reader{ reader }, _colours{ colours } { }

  auto Run(Room const room) -> Decoded<Picture> {
    if (auto const header{ Header(room) }; !header) return std::unexpected{ header.error() };
    if (_colours == Colours::SIXTEEN) std::ranges::generate(_runs, [this] { return _reader.NextWord(); });
    _units.assign(TOP + (_height + 1uz) * LINE_UNITS + 16, 0);
    _extents.assign(_height, 0);
    for (auto const row : Indices(_height)) {
      if (!DecodeRow(row)) return std::unexpected{ DecodeError::TRUNCATED };
    }
    return Picture{ .image = Pixels(), .length = _reader.Position() };
  }

  // The units every row wrote, each row from its first unit, as many as the longest row wrote.
  [[nodiscard]] auto Drawn() const -> Image {
    auto const units{ std::ranges::max(_extents) };
    auto       pixels{ std::vector<std::uint8_t>{} };
    auto       opaque{ std::vector<std::uint8_t>{} };
    for (auto const [row, x] : Grid(_height, units * 4)) {
      pixels.push_back(IndexOf(_units[TOP + row * LINE_UNITS + x / 4], x));
      opaque.push_back(x / 4 < _extents[row] ? 1 : 0);
    }
    return Image{ units * 4, _height, std::move(pixels), std::move(opaque) };
  }
  [[nodiscard]] auto Width() const noexcept -> std::size_t {
    return _width;
  }
  [[nodiscard]] auto Height() const noexcept -> std::size_t {
    return _height;
  }

private:
  // The size, refused as soon as a word of it is out of range or the picture leaves the room.
  auto Header(Room const room) -> Decoded<void> {
    _width = _reader.NextWord();
    if (_reader.Overrun()) return std::unexpected{ DecodeError::TRUNCATED };
    if (_width < 1 || _width > MAX_WIDTH) return std::unexpected{ DecodeError::BAD_HEADER };
    _height = _reader.NextWord();
    if (_reader.Overrun()) return std::unexpected{ DecodeError::TRUNCATED };
    if (_height < 1 || _height > MAX_HEIGHT || _width > room.width || _height > room.height) return std::unexpected{ DecodeError::BAD_HEADER };
    return {};
  }

  // Commands until the row's units are covered (the last may run past its end, never past the screen line) or the
  // stream runs out.
  auto DecodeRow(std::size_t const row) -> bool {
    auto const start{ TOP + row * LINE_UNITS };
    auto const end{ start + (std::size_t{ _width } + 3) / 4 };
    auto       at{ start };
    auto const commands{ std::views::iota(0uz) | std::views::take_while([&](std::size_t) { return at < end && !_reader.Overrun(); }) };
    std::ranges::for_each(commands, [&](std::size_t) {
      auto const command{ _reader.Next() };
      at += (command & 0x80) != 0 ? Copy(command, at) : Literal(command, at);
    });
    _extents[row] = std::min(at - start, LINE_UNITS);
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

  // The picture's own width and height, as palette indices.
  [[nodiscard]] auto Pixels() const -> Image {
    auto pixels{ std::vector<std::uint8_t>{} };
    pixels.reserve(std::size_t{ _width } * _height);
    for (auto const [row, x] : Grid(_height, _width)) pixels.push_back(IndexOf(_units[TOP + row * LINE_UNITS + x / 4], x));
    return Image{ _width, _height, std::move(pixels) };
  }

  Reader&                  _reader;
  Colours                  _colours;
  unsigned                 _width{};
  unsigned                 _height{};
  std::array<unsigned, 8>  _runs{};
  std::vector<unsigned>    _units;
  std::vector<std::size_t> _extents; // the units each row wrote
};

}

auto DecodePicture(Bytes const bytes, Colours const colours) -> Decoded<Picture> {
  auto reader{ ByteReader{ bytes } };
  return StreamDecoder{ reader, colours }.Run(Room{});
}

auto DecodePictureAsDrawn(NextByte const& next, Colours const colours, Room const room) -> Decoded<DrawnPicture> {
  auto reader{ SourceReader{ next } };
  auto decoder{
    StreamDecoder{ reader, colours }
  };
  return decoder.Run(room).transform([&](Picture const&) {
    return DrawnPicture{ .image = decoder.Drawn(), .width = decoder.Width(), .height = decoder.Height(), .length = reader.Position(), .fingerprint = reader.Fingerprinted() };
  });
}

}
