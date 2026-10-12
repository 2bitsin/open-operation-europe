#pragma once
// Why an asset pack, or one of its files, cannot be read.

#include <cstdint>
#include <expected>
#include <format>
#include <string>
#include <string_view>
#include <utility>

namespace opeu::assets::detail::pack_error {

enum class PackFault : std::uint8_t {
  MISSING_FILE,  // the pack has no file at that path
  BAD_ARCHIVE,   // the archive's header or directory is malformed
  BAD_MANIFEST,  // pack.json is not a manifest of this format
  UNKNOWN_ASSET, // no asset of that id
  BAD_IMAGE,     // an image is not a PNG, or not the size its entry states
};

struct PackError {
  PackFault   fault{};
  std::string detail; // the file, id or field concerned

  auto operator==(PackError const&) const -> bool = default;
};

template <class _Value>
using Packed = std::expected<_Value, PackError>;

[[nodiscard]] constexpr auto Describe(PackFault const fault) noexcept -> std::string_view {
  switch (fault) {
  case PackFault::MISSING_FILE:  return "missing file";
  case PackFault::BAD_ARCHIVE:   return "bad archive";
  case PackFault::BAD_MANIFEST:  return "bad manifest";
  case PackFault::UNKNOWN_ASSET: return "unknown asset";
  case PackFault::BAD_IMAGE:     return "bad image";
  }
  return "unknown";
}

[[nodiscard]] inline auto Describe(PackError const& error) -> std::string {
  return std::format("{}: {}", Describe(error.fault), error.detail);
}

[[nodiscard]] inline auto Fail(PackFault const fault, std::string detail) -> std::unexpected<PackError> {
  return std::unexpected{
    PackError{ .fault = fault, .detail = std::move(detail) }
  };
}

}

namespace opeu::assets {
using detail::pack_error::Describe;
using detail::pack_error::Fail;
using detail::pack_error::Packed;
using detail::pack_error::PackError;
using detail::pack_error::PackFault;
}
