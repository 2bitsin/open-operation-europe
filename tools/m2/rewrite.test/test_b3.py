"""The B3 pass on its fixture tree (fixtures/b3: the numbered sites already direct) and the sub-batch selection: the
callback parameters left unused go, the dispatchers and service ids nothing names go, the callback header goes from
files that name nothing of it."""
import shutil
import sys
from pathlib import Path, PurePosixPath

import pytest

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))

import rewrite  # noqa: E402
from rewriter.b3 import B3Pass, FindEnumerations  # noqa: E402
from rewriter.model import Model  # noqa: E402
from rewriter.source import SourceFile  # noqa: E402
from rewriter.substitute import _SideEffects  # noqa: E402
from rewriter.syntax import Span  # noqa: E402

FIXTURE = HERE / "fixtures" / "b3"
GAME = Path("sources/opeu/game")


@pytest.fixture(scope="module")
def passed(tmp_path_factory):
  tree = tmp_path_factory.mktemp("b3") / "tree"
  shutil.copytree(FIXTURE, tree)
  return tree, B3Pass(tree, {"SHOW", "DRAW", "WAIT"})


def Read(tree: Path, path: str) -> str:
  return (tree / GAME / path).read_text()


def test_a_callback_unused_or_only_handed_on_is_dropped_everywhere(passed):
  tree, result = passed
  menu = Read(tree, "core/menu.cpp")
  assert "void opeu_menu_show(Game& game, std::uint16_t row) {" in menu
  assert "void opeu_menu_run(Game& game) {\n  opeu_menu_show(game, 1);\n  opeu_menu_silent(game);\n}" in menu
  assert "opeu_menu_run(game);" in menu
  assert "void          opeu_menu_show(Game& game, std::uint16_t row);" in Read(tree, "core/menu.hpp")
  assert result.dropped == ["game/core/menu.cpp opeu_menu_run(event)", "game/core/menu.cpp opeu_menu_show(event)", "game/core/menu.cpp opeu_menu_silent(event)"]
  assert result.sites == 4


def test_a_void_statement_that_only_silenced_the_callback_goes_with_it(passed):
  tree, _ = passed
  assert "void opeu_menu_silent(Game& game) {\n}" in Read(tree, "core/menu.cpp")


def test_a_callback_still_called_or_of_a_routine_whose_address_is_taken_stays(passed):
  tree, result = passed
  menu = Read(tree, "core/menu.cpp")
  assert "void opeu_menu_hold(Game& game, HostEvent event) {" in menu and "opeu_menu_hold(game, svc_hold);" in menu
  assert "void opeu_menu_hooked(Game& game, HostEvent event) {" in menu
  assert any(line.startswith("game/core/menu.cpp opeu_menu_hold parameter 1: game/core/menu.cpp:") and "uses it" in line for line in result.kept)
  assert any(line.startswith("game/core/menu.cpp opeu_menu_hooked parameter 1: the routine's address is taken") for line in result.kept)


def test_a_dispatcher_nothing_names_goes_and_one_still_handed_on_stays(passed):
  tree, result = passed
  assert result.deleted == ["svc_menu"]
  assert "svc_menu" not in Read(tree, "core/menu.cpp") and "svc_menu" not in Read(tree, "core/menu.hpp")
  assert "std::uint16_t svc_hold(" in Read(tree, "core/menu.cpp")


def test_a_service_enumeration_nothing_names_goes_and_one_still_named_stays(passed):
  tree, result = passed
  menu = Read(tree, "core/menu.cpp")
  assert "enum Service" not in menu and "The services of the menu." not in menu
  assert "enum class HoldService" in menu
  assert result.enums == ["game/core/menu.cpp Service"]
  assert any(line.startswith("game/core/menu.cpp HoldService: HoldService is named at game/core/menu.cpp:") for line in result.kept)


def test_the_callback_header_goes_only_from_files_that_name_nothing_of_it(passed):
  tree, result = passed
  assert '#include "common/events.hpp"' not in Read(tree, "core/count.cpp")
  assert '#include "common/events.hpp"' in Read(tree, "core/draw.cpp")
  assert result.includes == ["game/core/count.cpp: common/events.hpp"]


def test_enumerations_are_read_with_their_enumerators():
  source = SourceFile(PurePosixPath("x.cpp"), "enum class Kind : std::uint8_t { A = 1, B, C = 4 };\nenum { X };\nint enumerate;\n")
  found = FindEnumerations(source)
  assert [(e.name, e.scoped, e.enumerators) for e in found] == [("Kind", True, ["A", "B", "C"]), ("", False, ["X"])]


def test_only_selects_the_sites_bound_to_dispatchers_of_the_given_files():
  model = Model(HERE / "fixtures" / "tree")
  everything = [s for s in model.substitutions.values() if not s.target.battle]
  chosen = rewrite._Selection(model, "units/*")
  assert chosen is not None and rewrite._Selection(model, "") is None
  picked = [s for s in everything if chosen(s)]
  assert picked and len(picked) < len(everything)
  assert all(str(model.dispatchers[s.target.callbacks[0]].function.file).startswith("game/units/") for s in picked)


@pytest.mark.parametrize("word, effects", [
  ("locals.At(FUEL_AT)", False),
  ("Near(data, data.text)", False),
  ("opeu_unit_side(unit)", True),
  ("count++", True),
])
def test_a_reserves_near_address_has_no_side_effect(word, effects):
  source = SourceFile(PurePosixPath("x.cpp"), word)
  assert _SideEffects(source, Span(0, len(source.tokens))) is effects
