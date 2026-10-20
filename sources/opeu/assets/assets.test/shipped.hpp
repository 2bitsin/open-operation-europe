#pragma once
// The shipped data files, as a source the decoders read: the 26 files game-data.test.hpp finds (the private
// repository's, or the directory OPEU_DATA_DIR names), the catalogue over them and the decoders' pack. A test of them
// skips without them. Each test opens its own: nothing is shared between tests.

#include "packs.hpp"

#include "opeu/game-data.test.hpp"

#include "opeu/assets/bytes.hpp"
#include "opeu/assets/catalogue-pack.hpp"
#include "opeu/assets/catalogue.hpp"
#include "opeu/assets/source.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <string_view>

namespace opeu::assets::test {
using namespace std::string_literals;
using namespace std::string_view_literals;

// The bytes of one of `data`'s files; empty when there is no such file.
[[nodiscard]] inline auto Shipped(Source const& data, std::string_view const name) -> Bytes {
  return data.File(name).value_or(Bytes{});
}

// A test of the shipped files: the files themselves, the catalogue over them, and the decoders' pack.
class ShippedTest : public ::opeu::test::GameDataTest {
protected:
  auto SetUp() -> void override {
    GameDataTest::SetUp();
    if (IsSkipped() || HasFailure()) return;
    _catalogue = std::make_unique<Catalogue const>(Data());
    _decoders  = std::make_unique<CataloguePack const>(Catalogue{ Data() });
  }

  [[nodiscard]] auto Shipped(std::string_view const name) const -> Bytes {
    return test::Shipped(Data(), name);
  }

  [[nodiscard]] auto Catalogued() const -> Catalogue const& {
    return *_catalogue;
  }

  [[nodiscard]] auto Decoders() const -> CataloguePack const& {
    return *_decoders;
  }

private:
  std::unique_ptr<Catalogue const>     _catalogue; // set by SetUp, or the test skipped
  std::unique_ptr<CataloguePack const> _decoders;  // likewise
};

// A test of the checked-in classic pack against the shipped files: the private repository's only.
class ClassicAgainstShippedTest : public ShippedTest {
protected:
  auto SetUp() -> void override {
    if (!::opeu::test::TestFile(CLASSIC_DIRECTORY)) GTEST_SKIP() << ::opeu::test::NoGameData("the checked-in classic pack");
    ShippedTest::SetUp();
  }
};

}
