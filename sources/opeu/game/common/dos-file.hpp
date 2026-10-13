// A file handle as the data segment keeps it: a word holding the DOS handle, -1 (0xffff) when no file is open.
#pragma once

#include "machine-files.hpp"

#include <concepts>
#include <cstdint>
#include <optional>
#include <utility>

namespace opeu::game::detail::dos_file {

inline constexpr std::int16_t NO_FILE{ -1 };

// The stored word of an open (or failed) open or create.
[[nodiscard]] constexpr auto StoredHandle(std::optional<services::FileHandle> handle) noexcept -> std::int16_t {
  return handle ? std::to_underlying(*handle) : NO_FILE;
}

// The handle a stored word names.
[[nodiscard]] constexpr auto HandleOf(std::integral auto stored) noexcept -> services::FileHandle {
  return services::FileHandle{ static_cast<std::int16_t>(stored) };
}

// The DOS answer of a read, a write or a seek: the count or position, -1 on an error.
template <typename _Value>
[[nodiscard]] constexpr auto DosResult(std::optional<_Value> result) noexcept -> _Value {
  return result.value_or(static_cast<_Value>(-1));
}

} // namespace opeu::game::detail::dos_file

namespace opeu::game {
using detail::dos_file::DosResult;
using detail::dos_file::HandleOf;
using detail::dos_file::NO_FILE;
using detail::dos_file::StoredHandle;
} // namespace opeu::game
