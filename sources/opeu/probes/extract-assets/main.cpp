// Diagnostic: extract-assets <assets dir> <out dir> writes every shipped asset as PNG under <out dir>/images and
// prints one JSON record per line (tools/assets/extract.py turns them into manifests).
#include "extractor.hpp"

#include "opeu/assets/source.hpp"

#include <print>
#include <span>
#include <string_view>

auto main(int const argc, char const* const* const argv) -> int {
  auto const args{
    std::span{ argv, static_cast<std::size_t>(argc) }
  };
  if (args.size() != 3) {
    std::println(stderr, "usage: extract-assets <assets dir> <out dir>");
    return 2;
  }
  auto const source{ opeu::assets::DirectorySource::Open(args[1]) };
  if (!source) {
    std::println(stderr, "{}", source.error());
    return 1;
  }
  auto const extractor{
    opeu::probes::extract_assets::Extractor{ *source, args[2],
                                            [](std::string_view const record) { std::println("{}", record); } }
  };
  if (auto const done{ extractor.Run() }; !done) {
    std::println(stderr, "{}", done.error());
    return 1;
  }
  return 0;
}
