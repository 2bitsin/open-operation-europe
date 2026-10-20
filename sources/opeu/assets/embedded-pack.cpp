#include "opeu/assets/embedded-pack.hpp"

#include "opeu-assets/resources.hpp"
#include "opeu/assets/fallback-pack.hpp"

#include <format>
#include <ranges>
#include <utility>

namespace opeu::assets::detail::embedded_pack {

using namespace std::string_literals;
using namespace std::string_view_literals;

auto EmbeddedPackFiles::Read(std::string_view const path) const -> Packed<std::vector<std::byte>> {
  auto const        name{ std::format("{}/{}", _directory, path) };
  auto const* const found{ resources::find(name) };
  if (found == nullptr) return Fail(PackFault::MISSING_FILE, name);
  return found->bytes() | std::ranges::to<std::vector>();
}

auto OpenLayered(std::unique_ptr<PackFiles const> classic, std::unique_ptr<PackFiles const> remaster) -> Packed<std::unique_ptr<AssetPack const>> {
  auto base{ Pack::Open(std::move(classic)) };
  if (!base) return std::unexpected{ base.error() };
  auto over{ Pack::Open(std::move(remaster)) };
  if (!over && over.error().fault == PackFault::MISSING_FILE) return std::make_unique<Pack const>(*std::move(base));
  if (!over) return std::unexpected{ over.error() };
  return std::make_unique<FallbackPack const>(std::make_unique<Pack const>(*std::move(over)), std::make_unique<Pack const>(*std::move(base)));
}

auto OpenEmbeddedPack(std::string_view const directory) -> Packed<Pack> {
  return Pack::Open(std::make_unique<EmbeddedPackFiles const>(directory));
}

auto OpenGamePack() -> Packed<std::unique_ptr<AssetPack const>> {
  return OpenLayered(std::make_unique<EmbeddedPackFiles const>(CLASSIC_PACK), std::make_unique<EmbeddedPackFiles const>(REMASTER_PACK));
}

}
