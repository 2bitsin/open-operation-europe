// The --wav recording: RIFF header and samples.
#include "wave-file.hpp"

#include "opl.hpp"

#include <algorithm>
#include <cstddef>
#include <ranges>
#include <utility>

namespace opeu::services::detail::wave_file {
namespace {

// `count` bytes of `value`, least significant first.
auto LittleEndian(std::uint32_t const value, int const count) -> std::string {
  return std::views::iota(0, count) | std::views::transform([value](int const i) { return static_cast<char>(value >> (8 * i)); }) | std::ranges::to<std::string>();
}

// The 44-byte RIFF header of 16-bit stereo PCM at the chip's rate.
auto WaveHeader(std::uint64_t const frames) -> std::string {
  auto const data{ static_cast<std::uint32_t>(frames * 4u) };
  auto const rate{ static_cast<std::uint32_t>(OPL_SAMPLE_RATE) };
  return "RIFF" + LittleEndian(data + 36, 4) + "WAVEfmt " + LittleEndian(16, 4) + LittleEndian(1, 2) /* PCM */ + LittleEndian(2, 2) /* stereo */
         + LittleEndian(rate, 4) + LittleEndian(rate * 4, 4) + LittleEndian(4, 2) /* block align */ + LittleEndian(16, 2) /* bits */ + "data" + LittleEndian(data, 4);
}

}

WaveFile::WaveFile(std::ofstream stream) : _stream{ std::move(stream) } { }

auto WaveFile::Create(std::string const& path) -> std::optional<WaveFile> {
  std::ofstream stream{ path, std::ios::binary };
  if (!stream) return std::nullopt;
  stream << WaveHeader(0);
  return WaveFile{ std::move(stream) };
}

auto WaveFile::Append(std::span<std::int16_t const> const stereo) -> void {
  auto const bytes{ std::as_bytes(stereo) };
  _bytes.resize(bytes.size());
  std::ranges::transform(bytes, _bytes.begin(), [](std::byte const b) { return static_cast<char>(b); });
  _stream.write(_bytes.data(), static_cast<std::streamsize>(_bytes.size()));
  _frames += stereo.size() / 2;
}

auto WaveFile::Finish() -> void {
  _stream.seekp(0);
  _stream << WaveHeader(_frames);
  _stream.close();
}

}
