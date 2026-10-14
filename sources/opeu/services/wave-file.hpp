#pragma once
// The --wav recording of the FM chip: a 16-bit stereo PCM RIFF file at the chip's rate. The header is written with a
// length of 0 and rewritten with the real length when the recording finishes.

#include <cstdint>
#include <fstream>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace opeu::services::detail::wave_file {

class WaveFile {
public:
  // The file at `path`, its header written; nothing when it cannot be created.
  [[nodiscard]] static auto Create(std::string const& path) -> std::optional<WaveFile>;

  // Appends interleaved stereo samples, as the host stores them.
  auto Append(std::span<std::int16_t const> stereo) -> void;
  // Rewrites the header with the recorded length and closes the file.
  auto Finish() -> void;

private:
  explicit WaveFile(std::ofstream stream);

  std::ofstream     _stream;
  std::uint64_t     _frames{};
  std::vector<char> _bytes; // the samples as the host stores them
};

}

namespace opeu::services {
using detail::wave_file::WaveFile;
}
