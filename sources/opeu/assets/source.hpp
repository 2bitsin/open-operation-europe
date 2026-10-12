#pragma once
// Where decoders get the shipped files from: the application's embedded table, a directory, or a test's fake.

#include "opeu/assets/image.hpp"

#include <expected>
#include <filesystem>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace opeu::assets::detail::source {

class Source {
public:
  virtual ~Source() = default;

  // The bytes of the file named as shipped ("FACE.DAT"), alive as long as the source; MISSING_FILE if absent.
  [[nodiscard]] virtual auto File(std::string_view name) const -> Decoded<Bytes> = 0;

protected:
  Source()                                     = default;
  Source(Source const&)                        = default;
  Source(Source&&) noexcept                    = default;
  auto operator=(Source const&) -> Source&     = default;
  auto operator=(Source&&) noexcept -> Source& = default;
};

// Files the caller already holds in memory (the application's embedded table, a test's fixtures), by name.
class TableSource final : public Source {
public:
  using Files = std::map<std::string, Bytes, std::less<>>;

  explicit TableSource(Files files) : _files{ std::move(files) } { }

  [[nodiscard]] auto File(std::string_view const name) const -> Decoded<Bytes> override {
    if (auto const found{ _files.find(name) }; found != _files.end()) return found->second;
    return std::unexpected{ DecodeError::MISSING_FILE };
  }

private:
  Files _files;
};

// Every file of a directory, read once into memory and owned by the source (the shipped set lives in
// sources/opeu/opeu/assets.embed/, the very bytes the application embeds).
class DirectorySource final : public Source {
public:
  [[nodiscard]] static auto Open(std::filesystem::path const& directory) -> std::expected<DirectorySource, std::string>;

  [[nodiscard]] auto File(std::string_view name) const -> Decoded<Bytes> override;
  [[nodiscard]] auto Names() const -> std::vector<std::string>;

private:
  using Contents = std::map<std::string, std::vector<std::byte>, std::less<>>;
  explicit DirectorySource(Contents contents) : _contents{ std::move(contents) } { }

  Contents _contents;
};

}

namespace opeu::assets {
using detail::source::DirectorySource;
using detail::source::Source;
using detail::source::TableSource;
}
