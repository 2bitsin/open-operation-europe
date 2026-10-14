#pragma once
// The shipped data files (the bytes the application embeds), as a source the decoders read. Tests run in the module
// directory, beside the application module that holds them. Each test opens its own: nothing is shared between tests.

#include "opeu/assets/catalogue-pack.hpp"
#include "opeu/assets/catalogue.hpp"
#include "opeu/assets/read-file.hpp"
#include "opeu/assets/source.hpp"

#include <cstddef>
#include <filesystem>
#include <string_view>
#include <vector>

namespace opeu::assets::test {

inline constexpr auto SHIPPED_DIRECTORY{ std::string_view{ "../opeu/assets.embed" } };

[[nodiscard]] inline auto Embedded() -> DirectorySource {
  return DirectorySource::Open(SHIPPED_DIRECTORY).value();
}

// The bytes of one shipped file; empty when there is no such file.
[[nodiscard]] inline auto Shipped(std::string_view const name) -> std::vector<std::byte> {
  return ReadFile(std::filesystem::path{ SHIPPED_DIRECTORY } / name).value_or(std::vector<std::byte>{});
}

// The shipped files and the catalogue over them, which reads them through a reference: one object, never copied.
class ShippedCatalogue {
public:
  ShippedCatalogue()                                           = default;
  ShippedCatalogue(ShippedCatalogue const&)                    = delete;
  ShippedCatalogue(ShippedCatalogue&&)                         = delete;
  auto operator=(ShippedCatalogue const&) -> ShippedCatalogue& = delete;
  auto operator=(ShippedCatalogue&&) -> ShippedCatalogue&      = delete;
  ~ShippedCatalogue()                                          = default;

  [[nodiscard]] auto Catalogued() const -> Catalogue const& {
    return _catalogue;
  }

private:
  DirectorySource _source{ Embedded() };
  Catalogue       _catalogue{ _source };
};

// The decoders' pack over the shipped files.
class ShippedPack {
public:
  ShippedPack()                                      = default;
  ShippedPack(ShippedPack const&)                    = delete;
  ShippedPack(ShippedPack&&)                         = delete;
  auto operator=(ShippedPack const&) -> ShippedPack& = delete;
  auto operator=(ShippedPack&&) -> ShippedPack&      = delete;
  ~ShippedPack()                                     = default;

  [[nodiscard]] auto Decoders() const -> CataloguePack const& {
    return _pack;
  }

private:
  DirectorySource _source{ Embedded() };
  CataloguePack   _pack{ Catalogue{ _source } };
};

}
