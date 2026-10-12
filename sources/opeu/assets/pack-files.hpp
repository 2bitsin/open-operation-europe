#pragma once
// Where a pack's files come from: a directory tree (development, remaster drop-ins), one archive file (shipping), or
// files already in memory (an embedded table, a test). Paths are relative and '/'-separated, as pack.json names them.

#include "opeu/assets/bytes.hpp"
#include "opeu/assets/pack-error.hpp"

#include <cstddef>
#include <filesystem>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace opeu::assets::detail::pack_files {

class PackFiles {
public:
  virtual ~PackFiles() = default;

  // The bytes of the file at `path`; MISSING_FILE if there is none.
  [[nodiscard]] virtual auto Read(std::string_view path) const -> Packed<std::vector<std::byte>> = 0;

protected:
  PackFiles()                                        = default;
  PackFiles(PackFiles const&)                        = default;
  PackFiles(PackFiles&&) noexcept                    = default;
  auto operator=(PackFiles const&) -> PackFiles&     = default;
  auto operator=(PackFiles&&) noexcept -> PackFiles& = default;
};

// The files under a directory, read when asked for. A path that leaves the directory is refused.
class DirectoryPackFiles final : public PackFiles {
public:
  explicit DirectoryPackFiles(std::filesystem::path root) : _root{ std::move(root) } { }

  [[nodiscard]] auto Read(std::string_view path) const -> Packed<std::vector<std::byte>> override;

private:
  std::filesystem::path _root;
};

// An .opeupack archive held in memory: the magic "OPEUPACK", the directory's length (32 bits, little-endian), the
// directory (JSON: every file's path, offset from the end of the directory, and size), then the files' bytes.
class ArchivePackFiles final : public PackFiles {
public:
  [[nodiscard]] static auto Open(std::filesystem::path const& path) -> Packed<ArchivePackFiles>;
  [[nodiscard]] static auto FromBytes(std::vector<std::byte> archive) -> Packed<ArchivePackFiles>;

  [[nodiscard]] auto Read(std::string_view path) const -> Packed<std::vector<std::byte>> override;
  [[nodiscard]] auto Paths() const -> std::vector<std::string>;

private:
  struct Extent {
    std::size_t at{};
    std::size_t size{};
  };
  using Extents = std::map<std::string, Extent, std::less<>>;

  ArchivePackFiles(std::vector<std::byte> archive, std::size_t body, Extents extents) : _archive{ std::move(archive) }, _body{ body }, _extents{ std::move(extents) } { }

  std::vector<std::byte> _archive;
  std::size_t            _body{}; // where the files' bytes start
  Extents                _extents;
};

// Files the caller holds, by path.
class MemoryPackFiles final : public PackFiles {
public:
  using Files = std::map<std::string, std::vector<std::byte>, std::less<>>;

  explicit MemoryPackFiles(Files files) : _files{ std::move(files) } { }

  [[nodiscard]] auto Read(std::string_view path) const -> Packed<std::vector<std::byte>> override;

private:
  Files _files;
};

}

namespace opeu::assets {
using detail::pack_files::ArchivePackFiles;
using detail::pack_files::DirectoryPackFiles;
using detail::pack_files::MemoryPackFiles;
using detail::pack_files::PackFiles;
}
