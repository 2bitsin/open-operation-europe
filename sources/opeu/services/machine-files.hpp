// What the game reads and writes: the DOS file handles of fileio.cpp (shipped files from memory, saved games in the
// save directory) and the disk reset.
#pragma once

#include "fmdriver.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace opeu::services::detail::machine_files {

enum class FileHandle : std::int16_t {
};

enum class OpenMode : std::uint16_t {
  READ       = 0,
  WRITE      = 1,
  READ_WRITE = 2,
};

enum class SeekOrigin : std::int16_t {
  START   = 0,
  CURRENT = 1,
  END     = 2,
};

// Far memory a read fills (until B7 replaces segment:offset with typed buffers).
struct FarRegion {
  FarAddress    at;
  std::uint16_t size{};
};

class Files {
public:
  Files()                                = default;
  Files(Files const&)                    = delete;
  Files(Files&&)                         = delete;
  auto operator=(Files const&) -> Files& = delete;
  auto operator=(Files&&) -> Files&      = delete;
  virtual ~Files()                       = default;

  // A handle, or none when the file cannot be opened or created.
  virtual auto Open(std::string_view name, OpenMode mode) -> std::optional<FileHandle> = 0;
  virtual auto Create(std::string_view name) -> std::optional<FileHandle>              = 0;
  virtual auto Exists(std::string_view name) -> bool                                   = 0;
  virtual auto Close(FileHandle file) -> void                                          = 0;
  // The bytes transferred, or none on an error.
  virtual auto Read(FileHandle file, std::span<std::uint8_t> into) -> std::optional<std::uint16_t>         = 0;
  virtual auto ReadFar(FileHandle file, FarRegion into) -> std::optional<std::uint16_t>                    = 0;
  virtual auto Write(FileHandle file, std::span<std::uint8_t const> bytes) -> std::optional<std::uint16_t> = 0;
  // The new position, or none on an error.
  virtual auto Seek(FileHandle file, std::int32_t position, SeekOrigin origin) -> std::optional<std::int32_t> = 0;
  // Writes every open file's pending bytes.
  virtual auto ResetDisk() -> void = 0;
};

}

namespace opeu::services {
using detail::machine_files::FarRegion;
using detail::machine_files::FileHandle;
using detail::machine_files::Files;
using detail::machine_files::OpenMode;
using detail::machine_files::SeekOrigin;
}
