#pragma once
// The 00/FF run-length code of the equipment silhouettes: a 0x00 or 0xFF byte is followed by a count n and stands
// for n + 1 copies of itself; any other byte stands for itself. The record ends when the output is full.

#include "opeu/assets/image.hpp"

#include <cstddef>
#include <vector>

namespace opeu::assets::detail::runs {

struct Expanded {
  std::vector<std::byte> bytes;
  std::size_t            consumed{}; // input bytes the record used
};

[[nodiscard]] auto ExpandRuns(Bytes bytes, std::size_t size) -> Decoded<Expanded>;

}

namespace opeu::assets {
using detail::runs::Expanded;
using detail::runs::ExpandRuns;
}
