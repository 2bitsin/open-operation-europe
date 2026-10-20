#include "opeu/assets/embedded-pack.hpp"

#include "opeu-assets/resources.hpp"
#include "opeu/assets/layered-pack.hpp"

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

auto OpenEmbeddedPack(std::string_view const directory) -> Packed<Pack> {
  return Pack::Open(std::make_unique<EmbeddedPackFiles const>(directory));
}

auto OpenGamePack() -> Packed<std::unique_ptr<AssetPack const>> {
  auto classic{ OpenEmbeddedPack(CLASSIC_PACK) };
  if (!classic) return std::unexpected{ classic.error() };
  return OpenLayered(std::make_unique<Pack const>(*std::move(classic)), std::make_unique<EmbeddedPackFiles const>(REMASTER_PACK));
}

}
