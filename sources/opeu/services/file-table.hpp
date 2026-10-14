#pragma once
// The job file table of the program: DOS handles over the platform's save files and the shipped data files compiled
// into the executable. fileio.cpp defines it and documents the DOS results; until services::Host owns it, fileio.cpp
// keeps the process's one behind the game-facing file functions (docs/platform-surface.md section 6).

#include "asset-table.hpp"
#include "platform.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace opeu::services::detail::file_table {

using platform::FileMode;
using platform::SaveFile;
using platform::SeekOrigin;

inline constexpr auto DOS_HANDLES{ 20uz };
inline constexpr auto DOS_FIRST_FILE{ 5uz };
inline constexpr auto NAME_LENGTH{ 79uz }; // the longest DOS name a handle remembers for its copy on write

// DOS error codes (INT 21h, carry set).
enum DosError : std::int16_t {
  INVALID_FUNCTION = 1,
  ACCESS_DENIED    = 5,
  INVALID_HANDLE   = 6,
  SEEK_ERROR       = 0x19,
};

// AL of the open call.
enum class Access : std::uint8_t {
  READ       = 0,
  WRITE      = 1,
  READ_WRITE = 2,
};

// A shipped file, read from the executable's copy; as on a file, a seek before the start fails and one past the end
// is allowed.
class ShippedFile {
public:
  explicit ShippedFile(std::span<std::uint8_t const> const bytes) : _bytes{ bytes } { }

  [[nodiscard]] auto Read(std::span<std::uint8_t> const into) -> std::size_t {
    if (_position < 0 || std::cmp_greater_equal(_position, _bytes.size())) return 0;
    auto const left{ _bytes.subspan(static_cast<std::size_t>(_position)) };
    auto const count{ std::min(into.size(), left.size()) };
    std::ranges::copy(left.first(count), into.begin());
    _position += static_cast<std::int64_t>(count);
    return count;
  }
  auto Seek(std::int64_t const offset, SeekOrigin const origin) -> std::optional<std::int64_t> {
    auto const base{ origin == SeekOrigin::START ? 0 : origin == SeekOrigin::CURRENT ? _position : static_cast<std::int64_t>(_bytes.size()) };
    if (base + offset < 0) return std::nullopt;
    _position = base + offset;
    return _position;
  }
  [[nodiscard]] auto Bytes() const -> std::span<std::uint8_t const> {
    return _bytes;
  }
  [[nodiscard]] auto Position() const -> std::int64_t {
    return _position;
  }

private:
  std::span<std::uint8_t const> _bytes;
  std::int64_t                  _position{ 0 };
};

// One DOS handle: a save-directory file or a shipped one.
struct OpenFile {
  std::variant<SaveFile, ShippedFile> source;
  std::string                         name; // the DOS name, for a copy on write
  Access                              access{ Access::READ };
  bool                                last_write{ false }; // the previous transfer was a write
};

// The job file table of the program.
class FileTable {
public:
  auto UseAssets(AssetLookup lookup) -> void {
    _find = std::move(lookup);
  }
  [[nodiscard]] auto Open(std::string_view name, std::uint16_t mode) -> std::int16_t;
  [[nodiscard]] auto Create(std::string_view name) -> std::int16_t;
  auto               Close(std::int16_t handle) -> void;
  [[nodiscard]] auto Read(std::int16_t handle, std::span<std::uint8_t> bytes) -> std::int16_t;
  [[nodiscard]] auto Write(std::int16_t handle, std::span<std::uint8_t const> bytes) -> std::int16_t;
  [[nodiscard]] auto Seek(std::int16_t handle, std::int32_t position, std::int16_t origin) -> std::int32_t;
  auto               FlushAll() -> void;

private:
  [[nodiscard]] auto Taken(std::int16_t handle) -> std::optional<OpenFile>&;
  [[nodiscard]] auto Valid(std::int16_t handle) -> bool;
  [[nodiscard]] auto Lookup(std::string_view name) const -> std::optional<std::span<std::uint8_t const>>;
  [[nodiscard]] auto NewHandle(OpenFile file) -> std::int16_t;
  auto               SwitchDirection(OpenFile& file, bool writing) -> void;
  [[nodiscard]] auto CopyOnWrite(OpenFile& file) -> bool;

  std::array<std::optional<OpenFile>, DOS_HANDLES> _handles;
  AssetLookup                                      _find;
};

}

namespace opeu::services {
using detail::file_table::FileTable;
}
