#pragma once
// Which pack image the bytes the game hands over are: the content index's answer (assets::ContentIndex, by the bytes'
// fingerprint) when the pack holds that id. The answers are remembered by the bytes themselves: a quick key of their
// shape and a few sampled bytes finds the candidates, comparing every byte confirms one, so the same tile drawn again
// costs a comparison, not a fingerprint and an index search.

#include "opeu/assets/content-index.hpp"
#include "opeu/assets/pack.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace opeu::render::detail::recognised_images {

// How the images the game handed over were found in the pack.
struct ByteStatistics {
  std::size_t recognised{}; // known by their bytes and in the pack
  std::size_t unpacked{};   // known by their bytes, but the pack lacks the id

  auto operator==(ByteStatistics const&) const -> bool = default;
};

class RecognisedImages {
public:
  // The answers of `index`, kept when `pack` holds the id; both outlive this.
  RecognisedImages(assets::AssetPack const& pack, assets::ContentIndex const& index);

  // The pack id of a raw image of `shape` drawn from `bytes`; none when the bytes are no shipped asset (or end first),
  // or the pack lacks it.
  [[nodiscard]] auto Raw(assets::RawShape const& shape, std::span<std::uint8_t const> bytes) -> std::optional<std::string_view>;
  // The same for a picture stream read as `bytes`.
  [[nodiscard]] auto Stream(assets::Layout layout, std::span<std::uint8_t const> bytes) -> std::optional<std::string_view>;
  [[nodiscard]] auto Statistics() const noexcept -> ByteStatistics {
    return _statistics;
  }

private:
  // The shape of some bytes and a few of them: what finds the remembered answers worth comparing.
  struct QuickKey {
    assets::RawShape shape;
    std::size_t      size{};
    std::uint64_t    sample{};

    auto operator==(QuickKey const& other) const noexcept -> bool;
  };
  struct QuickHash {
    auto operator()(QuickKey const& key) const noexcept -> std::size_t;
  };
  // An answer: the bytes it is for, and the id when the index knew them (`unpacked` when the pack lacked it).
  struct Answer {
    std::vector<std::uint8_t>       bytes;
    std::optional<std::string_view> id;
    bool                            unpacked{};
  };

  [[nodiscard]] auto Remembered(QuickKey const& quick, std::span<std::uint8_t const> bytes) -> Answer const&;
  [[nodiscard]] auto Asked(assets::ContentKey const& key) const -> Answer;
  [[nodiscard]] auto Counted(Answer const& answer) -> std::optional<std::string_view>;

  std::reference_wrapper<assets::AssetPack const>              _pack;
  std::reference_wrapper<assets::ContentIndex const>           _index;
  std::unordered_map<QuickKey, std::vector<Answer>, QuickHash> _answers;
  std::size_t                                                  _remembered{};
  ByteStatistics                                               _statistics;
};

}

namespace opeu::render {
using detail::recognised_images::ByteStatistics;
using detail::recognised_images::RecognisedImages;
}
