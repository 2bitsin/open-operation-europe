#include "opeu/assets/runs.hpp"

#include <algorithm>
#include <iterator>
#include <ranges>
#include <utility>

namespace opeu::assets::detail::runs {

auto ExpandRuns(Bytes const bytes, std::size_t const size) -> Decoded<Expanded> {
  auto reader{ ByteReader{ bytes } };
  auto out{ std::vector<std::byte>{} };
  out.reserve(size);
  auto const codes{ std::views::iota(0uz)
                    | std::views::take_while([&](std::size_t) { return out.size() < size && !reader.Overrun(); }) };
  std::ranges::for_each(codes, [&](std::size_t) {
    auto const value{ reader.Next() };
    auto const count{ value == 0x00 || value == 0xFF ? reader.Next() + 1uz : 1uz };
    if (reader.Overrun()) return;
    std::ranges::copy(std::views::repeat(std::byte{ value }, std::min(count, size - out.size())),
                      std::back_inserter(out));
  });
  if (reader.Overrun()) return std::unexpected{ DecodeError::TRUNCATED };
  return Expanded{ .bytes = std::move(out), .consumed = reader.Position() };
}

}
