"""The B4 pass's pieces on small inputs: the code enums keep the original far address as their value, the data
initialisers' {offset, segment} pairs become enumerators (and a pair no enumerator names is a problem, left as it was),
the slots of data-segment.hpp take the enums, and the link table's handlers are deleted."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))

from rewriter.b4 import (ALL_ENUMS, PRESENTER, SLOT_ENUMS, CheckAgainstLinks, DeleteHandlers, EnumBlock, LinkEntries,  # noqa: E402
                         RewriteInitialisers, RewriteSegmentHeader)


def Enum(name: str):
  return next(enum for enum in ALL_ENUMS if enum.name == name)


def test_an_enumerator_is_the_far_address_seg_shl_16_or_off():
  effect = next(e for e in Enum("BattleHandler").enumerators if e.name == "EFFECT_9")
  assert effect.value == 0x0d920050
  assert "  EFFECT_9 = 0x0d920050, " not in EnumBlock(Enum("BattleHandler"))
  assert "  EFFECT_9 = 0x0d920050,\n" in EnumBlock(Enum("BattleHandler"))


def test_every_enum_is_a_32_bit_enum_class_with_unique_names_and_values():
  for enum in ALL_ENUMS:
    assert f"enum class {enum.name} : std::uint32_t {{" in EnumBlock(enum)
    assert len({e.name for e in enum.enumerators}) == len(enum.enumerators)
    assert len({e.value for e in enum.enumerators}) == len(enum.enumerators)
  assert PRESENTER not in SLOT_ENUMS


def test_table_pairs_become_enumerators():
  text = "  data.battle_handlers = { {\n    { 0x0008, 0x0d92 },\n    { 0x0050, 0x0d92 },\n  } };\n"
  rewritten, replaced, problems = RewriteInitialisers("game/data/x.cpp", text, set())
  assert rewritten == "  data.battle_handlers = { {\n    BattleHandler::EFFECT_0,\n    BattleHandler::EFFECT_9,\n  } };\n"
  assert (replaced, problems) == (2, [])


def test_a_pair_no_enumerator_names_is_a_problem_and_stays():
  text = "  data.battle_handlers = { {\n    { 0x0009, 0x0d92 },\n  } };\n"
  rewritten, replaced, problems = RewriteInitialisers("game/data/x.cpp", text, set())
  assert "{ 0x0009, 0x0d92 }" in rewritten
  assert replaced == 0
  assert problems == ["game/data/x.cpp: battle_handlers: {0x0009, 0x0d92} is no BattleHandler"]


def test_a_list_windows_last_two_pairs_are_its_draw_and_select():
  text = "  data.people_dialog = { 1, 2, 3, 4, 0x10, 5, { 0x121e, 0x06db }, { 0x0000, 0x0000 } };\n"
  rewritten, replaced, _ = RewriteInitialisers("game/data/x.cpp", text, {"people_dialog"})
  assert rewritten == "  data.people_dialog = { 1, 2, 3, 4, 0x10, 5, ListDraw::GENERALS, ListSelect::NONE };\n"
  assert replaced == 2
  assert RewriteInitialisers("game/data/x.cpp", text, set())[1] == 0  # not a window slot: left alone


def test_the_slots_take_the_enums_and_are_re_exported():
  fields = "".join(f"  FarPointer {name};\n" for enum in SLOT_ENUMS for name in enum.fields)
  text = (f"struct FarPointer {{\n  std::uint16_t offset;\n}};\nstruct DataSegment {{\n{fields}  FarPointer news;\n}};\n"
          "using detail::data_segment::FarPointer;\n")
  rewritten, problems = RewriteSegmentHeader(text)
  assert problems == []
  assert "  BattleHandler battle_handlers;\n" in rewritten
  assert "  FarPointer news;\n" in rewritten
  assert "using detail::data_segment::FarPointer;\nusing detail::data_segment::NoonAction;\n" in rewritten


def test_the_handlers_of_the_link_table_are_deleted():
  text = ("void opeu_a(Game& game) {\n}\n\n"
          "auto port_routine_opeu_a(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {\n  opeu_a(game);\n  return 0;\n}\n\n"
          "void opeu_b(Game& game) {\n}\n")
  rewritten, count = DeleteHandlers(text)
  assert count == 1
  assert rewritten == "void opeu_a(Game& game) {\n}\n\nvoid opeu_b(Game& game) {\n}\n"


def test_an_enumerator_must_name_the_routine_the_link_table_holds_at_its_address():
  table = ('  { 0x0d928u, "opeu_sound_cue0", port_routine_opeu_sound_cue0 },\n'
           '  { 0x0d930u, "opeu_other", nullptr },\n')
  entries = LinkEntries(table)
  assert entries[0x0d928] == ("opeu_sound_cue0", "port_routine_opeu_sound_cue0")
  problems = CheckAgainstLinks(entries)
  assert not any("EFFECT_0 " in problem for problem in problems)
  assert any(problem.startswith("BattleHandler::EFFECT_1 0d92:0010: the link table holds ('opeu_other', None)") for problem in problems)
