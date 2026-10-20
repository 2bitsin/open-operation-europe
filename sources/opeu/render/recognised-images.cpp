#include "opeu/render/recognised-images.hpp"

#include "opeu/assets/fingerprint.hpp"

#include <algorithm>
#include <ranges>
#include <utility>

namespace opeu::render::detail::recognised_images {
namespace {

constexpr auto REMEMBERED{ 8192uz }; // answers kept; past it, all are forgotten and asked again
constexpr auto SAMPLES{ 16uz };      // bytes sampled for the quick key

[[nodiscard]] constexpr auto PlanesOf(assets::Layout const layout) noexcept -> std::size_t {
  if (layout == assets::Layout::PLANES_4) return 4;
  if (layout == assets::Layout::PLANES_3) return 3;
  return 1;
}

// Sixteen bytes spread over `bytes`, mixed into one word.
[[nodiscard]] auto Sampled(std::span<std::uint8_t const> const bytes) -> std::uint64_t {
  if (bytes.empty()) return 0;
  auto const at{ [&bytes](std::size_t const n) { return bytes[n * (bytes.size() - 1) / (SAMPLES - 1)]; } };
  return std::ranges::fold_left(std::views::iota(0uz, SAMPLES) | std::views::transform(at), std::uint64_t{ 0 },
                                [](std::uint64_t const word, std::uint8_t const byte) { return (word * 0x100000001B3) ^ byte; });
}

}

auto RecognisedImages::QuickKey::operator==(QuickKey const& other) const noexcept -> bool {
  return shape.layout == other.shape.layout && shape.width_bytes == other.shape.width_bytes && shape.height == other.shape.height && size == other.size && sample == other.sample;
}

auto RecognisedImages::QuickHash::operator()(QuickKey const& key) const noexcept -> std::size_t {
  auto const shape{ (std::uint64_t{ key.size } << 32U) ^ (std::uint64_t{ key.shape.height } << 16U) ^ (std::uint64_t{ key.shape.width_bytes } << 8U) ^ std::to_underlying(key.shape.layout) };
  return std::hash<std::uint64_t>{}(key.sample ^ shape);
}

RecognisedImages::RecognisedImages(assets::AssetPack const& pack, assets::ContentIndex const& index) : _pack{ pack }, _index{ index } { }

auto RecognisedImages::Raw(assets::RawShape const& shape, std::span<std::uint8_t const> const bytes) -> std::optional<std::string_view> {
  auto const size{ shape.width_bytes * shape.height * PlanesOf(shape.layout) };
  if (bytes.size() < size) return std::nullopt;
  auto const drawn{ bytes.first(size) };
  return Counted(Remembered(QuickKey{ .shape = shape, .size = size, .sample = Sampled(drawn) }, drawn));
}

auto RecognisedImages::Stream(assets::Layout const layout, std::span<std::uint8_t const> const bytes) -> std::optional<std::string_view> {
  return Counted(Asked(assets::StreamKey(layout, assets::FingerprintOf(std::as_bytes(bytes)))));
}

// The answer for `bytes`: remembered, or asked of the index and remembered.
auto RecognisedImages::Remembered(QuickKey const& quick, std::span<std::uint8_t const> const bytes) -> Answer const& {
  auto& candidates{ _answers[quick] };
  if (auto const found{ std::ranges::find_if(candidates, [bytes](Answer const& answer) { return std::ranges::equal(answer.bytes, bytes); }) }; found != candidates.end()) return *found;
  if (_remembered >= REMEMBERED) {
    _answers.clear();
    _remembered = 0;
  }
  auto answer{ Asked(assets::RawKey(quick.shape, std::as_bytes(bytes))) };
  answer.bytes = bytes | std::ranges::to<std::vector>();
  ++_remembered;
  return _answers[quick].emplace_back(std::move(answer));
}

auto RecognisedImages::Asked(assets::ContentKey const& key) const -> Answer {
  auto const id{ _index.get().Recognise(key) };
  if (!id) return Answer{};
  auto const packed{ _pack.get().Lists(*id) };
  return Answer{ .bytes = {}, .id = packed ? id : std::nullopt, .unpacked = !packed };
}

auto RecognisedImages::Counted(Answer const& answer) -> std::optional<std::string_view> {
  if (answer.unpacked) ++_statistics.unpacked;
  if (answer.id) ++_statistics.recognised;
  return answer.id;
}

}
