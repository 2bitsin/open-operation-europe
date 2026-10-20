#pragma once
// The packs compiled into the executable (packs.embed/): the classic pack, the shipped pictures with readable file
// names, and the remaster beside it, which holds the same file names at a higher scale for the ids it replaces. The
// game draws the remaster over the classic pack; with no remaster embedded it draws the classic pack alone.

#include "opeu/assets/pack-files.hpp"
#include "opeu/assets/pack.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace opeu::assets::detail::embedded_pack {

using namespace std::string_literals;
using namespace std::string_view_literals;

inline constexpr auto CLASSIC_PACK{ "classic"sv };
inline constexpr auto REMASTER_PACK{ "remaster"sv };

// The files of one embedded pack: `directory` is the pack's directory under packs.embed/.
class EmbeddedPackFiles final : public PackFiles {
public:
  explicit EmbeddedPackFiles(std::string_view const directory) : _directory{ directory } { }

  [[nodiscard]] auto Read(std::string_view path) const -> Packed<std::vector<std::byte>> override;

private:
  std::string _directory;
};

// A remaster over the classic pack, or the classic pack alone when the remaster's files hold no pack.json. A remaster
// that has a manifest but cannot be read is an error, not a silent fall back.
[[nodiscard]] auto OpenLayered(std::unique_ptr<PackFiles const> classic, std::unique_ptr<PackFiles const> remaster) -> Packed<std::unique_ptr<AssetPack const>>;

// The embedded pack in packs.embed/<directory>/.
[[nodiscard]] auto OpenEmbeddedPack(std::string_view directory) -> Packed<Pack>;

// The pack the game draws: the embedded remaster over the embedded classic pack.
[[nodiscard]] auto OpenGamePack() -> Packed<std::unique_ptr<AssetPack const>>;

}

namespace opeu::assets {
using detail::embedded_pack::CLASSIC_PACK;
using detail::embedded_pack::EmbeddedPackFiles;
using detail::embedded_pack::OpenEmbeddedPack;
using detail::embedded_pack::OpenGamePack;
using detail::embedded_pack::OpenLayered;
using detail::embedded_pack::REMASTER_PACK;
}
