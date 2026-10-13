"""The B2 pass on its fixture tree (fixtures/b2: battle sites already direct) and the text rules the battle sites
introduced: casts written as static_cast, discarded results, include placement, comments that go with what they
describe, function pointer members."""
import shutil
import sys
from pathlib import Path, PurePosixPath

import pytest

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))

from rewriter.apply import _Includes  # noqa: E402
from rewriter.b2 import ApplyEdits, B2Pass  # noqa: E402
from rewriter.casts import ModernCasts  # noqa: E402
from rewriter.functions import ScanFunctions  # noqa: E402
from rewriter.source import SourceFile  # noqa: E402
from rewriter.substitute import Discarded, _NarrowedToByte  # noqa: E402
from rewriter.syntax import Span  # noqa: E402

FIXTURE = HERE / "fixtures" / "b2"
GAME = Path("sources/opeu/game")


@pytest.fixture(scope="module")
def passed(tmp_path_factory):
  tree = tmp_path_factory.mktemp("b2") / "tree"
  shutil.copytree(FIXTURE, tree)
  return tree, B2Pass(tree)


def Read(tree: Path, path: str) -> str:
  return (tree / GAME / path).read_text()


def test_a_battle_event_only_handed_on_is_dropped_everywhere(passed):
  tree, result = passed
  battle = Read(tree, "map/battle.cpp")
  assert "void opeu_step(Game& game, std::uint16_t element) {\n  opeu_mark(game, element);" in battle
  assert "void opeu_mark(Game& game, std::uint16_t cell) {" in battle
  assert "opeu_battle_turn(game, 3);" in battle
  assert "void          opeu_show(Game& game);" in Read(tree, "map/battle.hpp")
  assert result.dropped == ["game/map/battle.cpp opeu_battle_turn(event)", "game/map/battle.cpp opeu_mark(BattleEvent)",
                            "game/map/battle.cpp opeu_show(event)", "game/map/battle.cpp opeu_step(event)"]
  assert result.sites == 8


def test_a_battle_event_of_a_routine_whose_address_is_taken_stays(passed):
  tree, result = passed
  assert "void opeu_hooked(Game& game, BattleEvent event) {" in Read(tree, "core/hook.cpp")
  assert "game/core/hook.cpp opeu_hooked parameter 1: the routine's address is taken (game/core/hook.cpp:19)" in result.kept
  assert "using BattleEvent" in Read(tree, "common/events.hpp")


def test_a_forwarder_nothing_names_goes_with_what_only_it_named(passed):
  tree, result = passed
  assert result.deleted == ["port_service", "svc_battle"]
  assert "svc_battle" not in Read(tree, "map/battle.hpp")
  assert "services of this module" not in Read(tree, "map/battle.hpp")
  link = Read(tree, "link/link.hpp")
  assert "port_service" not in link and "address-keyed" not in link and "/* Call a routine by its identifier. */" in link
  assert "port_service" not in Read(tree, "link/link.cpp")


def test_a_handler_nothing_reaches_is_unlinked_and_one_a_far_pointer_can_hold_stays(passed):
  tree, result = passed
  assert result.handlers == ["port_routine_opeu_step", "port_routine_opeu_mark"]
  battle = Read(tree, "map/battle.cpp")
  assert "port_routine_opeu_step" not in battle and "The step through the link table" not in battle
  assert "auto port_routine_opeu_show(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {\n  opeu_show(game);" in battle
  table = Read(tree, "link/gen/link-table.cpp")
  assert '{ 0x01000u, "opeu_step", nullptr },' in table and "PortHandler port_routine_opeu_step;" not in table
  assert '{ 0x01020u, "opeu_show", port_routine_opeu_show },' in table
  assert result.kept[-1] == "port_routine_opeu_show: a far pointer {0x0020, SHOW_SEGMENT} at game/core/hook.cpp:14 can hold it"


def test_keys_nothing_names_go_and_an_empty_enumeration_takes_its_header(passed):
  tree, result = passed
  assert result.keys == ["MARK", "SHOW", "STEP", "common/battle-routine.hpp"]
  assert not (tree / GAME / "common/battle-routine.hpp").exists()
  battle = Read(tree, "map/battle.cpp")
  assert '#include "common/battle-routine.hpp"' not in battle and '#include "link/link.hpp"' not in battle
  assert result.includes == ["game/map/battle.cpp: common/battle-routine.hpp (deleted)", "game/map/battle.cpp: link/link.hpp"]


@pytest.mark.parametrize("text, modern", [
  ("f(game, (TextHost*)port_textout_host(), (short)1)",  "f(game, static_cast<TextHost*>(port_textout_host()), static_cast<short>(1))"),
  ("f((uint8_t)x.y[2], (const UiHost*)host())",          "f(static_cast<uint8_t>(x.y[2]), static_cast<const UiHost*>(host()))"),
  ("(uint32_t)a << 16 | b",                              "static_cast<uint32_t>(a) << 16 | b"),
  ("(uint16_t)(a + b) * 2",                              "static_cast<uint16_t>(a + b) * 2"),
  ("(void)f(x)",                                         "static_cast<void>(f(x))"),
  ("(uint8_t)(uint16_t)x",                               "(uint8_t)(uint16_t)x"),
  ("g((short)-1)",                                       "g((short)-1)"),
  ("(Name)x",                                            "(Name)x"),
  ("sizeof(int) + h(a)(b)",                              "sizeof(int) + h(a)(b)"),
  ("(uint8_t)static_cast<std::uint16_t>(f(x))",          "static_cast<uint8_t>(static_cast<std::uint16_t>(f(x)))"),
  ("(const char*)bytes.data()",                          "reinterpret_cast<const char*>(bytes.data())"),
  ("(uint8_t*)p",                                        "reinterpret_cast<uint8_t*>(p)"),
])
def test_c_style_casts_of_postfix_operands_become_static_casts(text, modern):
  assert ModernCasts(text) == modern


@pytest.mark.parametrize("text, discarded", [
  ("(uint16_t)opeu_vga_clear()",            "opeu_vga_clear()"),
  ("static_cast<std::uint16_t>(f(a, (b)))", "f(a, (b))"),
  ("(uint16_t)f(a) + g()",                  "(uint16_t)f(a) + g()"),
  ("f(x)",                                  "f(x)"),
])
def test_a_discarded_result_loses_its_conversion(text, discarded):
  assert Discarded(text) == discarded


def test_a_game_header_joins_the_first_include_block_and_a_bare_one_the_glue_block():
  text = '#include "map/a.hpp"\n#include "types.hpp"\n\n#include "common/game.hpp"\n#include "port.hpp"\n\n#include <array>\n'
  edits = _Includes(text, ["map/b.hpp", "fmdriver.hpp", "<span>"])
  for at, _, inserted in sorted(edits, reverse=True):
    text = text[:at] + inserted + text[at:]
  assert text == ('#include "map/a.hpp"\n#include "types.hpp"\n#include "map/b.hpp"\n\n#include "common/game.hpp"\n#include "port.hpp"\n'
                  '#include "fmdriver.hpp"\n\n#include <array>\n#include <span>\n')


def test_a_deleted_declaration_takes_its_own_comment_but_not_a_groups():
  text = "// Group.\nvoid a();\nvoid b();\n\n/* Only c. */\nvoid c();\n/* d */\nvoid d();\n"
  assert ApplyEdits(text, {("lines", 2, 2), ("lines", 6, 6)}) == "// Group.\nvoid b();\n\n/* d */\nvoid d();\n"


def test_a_paragraph_whose_code_all_goes_takes_its_comments_and_a_blank_line():
  text = "void a();\n\n// The services.\nvoid s();\nvoid t();\n\nvoid z();\n"
  assert ApplyEdits(text, {("lines", 4, 5)}) == "void a();\n\nvoid z();\n"


def test_a_function_pointer_member_is_not_a_function_named_by_its_type():
  source = SourceFile(PurePosixPath("x.hpp"), "struct Driver {\n  uint8_t (*read_status)(void* user);\n};\nvoid real(int);\n")
  assert [function.name for function in ScanFunctions(source).declarations] == ["real"]


@pytest.mark.parametrize("code, narrowed", [
  ("auto x{ static_cast<uint8_t>(Call(game, e, K)) };",      True),
  ("auto x{ static_cast<std::uint8_t>(Call(game, e, K)) };", True),
  ("auto x{ (uint8_t)Call(game, e, K) };",                   True),
  ("auto x{ static_cast<uint8_t>(Call(game, e, K) + 1) };",  False),
  ("auto x{ Call(game, e, K) };",                            False),
])
def test_a_result_narrowed_to_a_byte_needs_no_16_bit_conversion(code, narrowed):
  source = SourceFile(PurePosixPath("x.cpp"), code)
  first = next(i for i, token in enumerate(source.tokens) if token.text == "Call")
  assert _NarrowedToByte(source, Span(first, source.partners[first + 1] + 1)) is narrowed
