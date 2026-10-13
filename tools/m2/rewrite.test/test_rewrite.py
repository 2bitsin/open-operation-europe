"""The M2 rewriter on its fixture tree: the dispatch model, each substitution rule, the review listings, the scratch
apply, the B1 pass (the game for the data segment) and the B4 model."""
import json
import shutil
import sys
from pathlib import Path, PurePosixPath

import pytest

HERE = Path(__file__).resolve().parent
TOOL = HERE.parent
sys.path.insert(0, str(TOOL))

import rewrite  # noqa: E402
from rewriter import report  # noqa: E402
from rewriter.apply import Apply, CheckScratch, MainCheckout  # noqa: E402
from rewriter.calls import ValueUsed  # noqa: E402
from rewriter.dispatch import LINKING, MULTI, RAW, SINGLE  # noqa: E402
from rewriter.model import Model  # noqa: E402
from rewriter.source import ROOT, SourceFile  # noqa: E402
from rewriter.b1 import B1Pass, RewriteFile, WriteB1  # noqa: E402
from rewriter.source import SourceTree  # noqa: E402
from rewriter.stubs import B4Model  # noqa: E402
from rewriter.syntax import Compact, Span, Statements  # noqa: E402

FIXTURE = HERE / "fixtures" / "tree"


@pytest.fixture(scope="module")
def model() -> Model:
  return Model(FIXTURE)


def Replacement(model: Model, file: str, line: int, index: int = 0) -> str:
  found = [s for s in model.substitutions.values() if str(s.site.file) == file and s.site.line == line]
  return " ".join(found[index].text.split())


def Kinds(model: Model, name: str) -> dict[str, str]:
  return {case.labels[0]: case.kind for case in model.dispatchers[name].cases}


def test_statements_end_at_semicolons_and_blocks():
  source = SourceFile(PurePosixPath("x.cpp"), "a(); if (b) { c(); } else { d(); } auto e{ f() }; do { g(); } while (h);")
  parts = [Compact(source.tokens, span) for span in Statements(source.tokens, source.partners, Span(0, len(source.tokens)))]
  assert parts == ["a();", "if(b){c();}else{d();}", "auto e{f()};", "do{g();}while(h);"]


@pytest.mark.parametrize("code, used", [
  ("void f() { Call(ds, e, 1); }",              False),
  ("void f() { if (x) Call(ds, e, 1); }",       False),
  ("void f() { auto y{ Call(ds, e, 1) }; }",    True),
  ("void f() { return Call(ds, e, 1); }",       True),
  ("void f() { for (; Call(ds, e, 1), y;) {} }", False),
  ("void f() { again: Call(ds, e, 1); }",        False),
  ("void f() { x = y ? z : Call(ds, e, 1); }",   True),
])
def test_a_value_is_used_unless_the_call_is_a_whole_statement(code, used):
  source = SourceFile(PurePosixPath("x.cpp"), code)
  first = next(i for i, token in enumerate(source.tokens) if token.text == "Call")
  assert ValueUsed(source, Span(first, source.partners[first + 1] + 1)) is used


def test_cases_are_classified_by_shape(model):
  assert Kinds(model, "svc_alpha") == {"SHOW": SINGLE, "LOOKUP": SINGLE, "WIDTH": SINGLE, "FORMAT": SINGLE, "PAIR": SINGLE,
                                       "TWICE": MULTI, "WRITE_BACK": RAW, "HOST": SINGLE, "FAR_ACTION": LINKING}
  assert model.dispatchers["svc_beta"].forward.target == "svc_alpha"
  assert model.dispatchers["svc_battle"].battle
  assert model.dispatchers["svc_battle"].cases[0].forward.reorder == (1, 0)


def test_handlers_list_their_words_in_argument_order(model, tmp_path):
  document = report.WriteModel(model, tmp_path)
  handlers = {handler["name"]: handler for handler in document["handlers"]}
  assert handlers["port_routine_opeu_grid_step"]["argument_words"] == [1, 0]
  assert handlers["port_routine_opeu_grid_step"]["keys"] == ["GRID_STEP"]
  assert handlers["port_routine_opeu_set_order"]["argument_words"] == [0, 1]


def test_words_keep_their_16_bit_conversion_only_where_the_place_is_wider(model):
  assert Replacement(model, "game/core/alpha.cpp", 10) == "opeu_show(ds, x, static_cast<std::uint8_t>(7))"
  assert Replacement(model, "game/core/alpha.cpp", 11) == "opeu_width(ds, static_cast<std::uint16_t>(wide))"


def test_a_signed_result_is_converted_as_ax_gave_it(model):
  assert Replacement(model, "game/core/alpha.cpp", 12) == "static_cast<std::uint16_t>(opeu_lookup(ds, static_cast<std::int16_t>(signed_word)))"


def test_the_word_span_becomes_an_array_of_the_words(model):
  assert Replacement(model, "game/core/alpha.cpp", 13) == (
    "opeu_format(ds, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(width) })")


def test_reordered_words_with_side_effects_are_hoisted_in_order(model):
  substitution = next(s for s in model.substitutions.values() if s.site.line == 14 and str(s.site.file) == "game/core/alpha.cpp")
  assert substitution.text == "opeu_pair(ds, hoisted1, hoisted0)"
  assert substitution.hoisted == [(0, "opeu_tick(ds)"), (1, "opeu_tick(ds)")]


def test_a_word_read_past_the_end_becomes_zero_and_is_listed(model):
  assert Replacement(model, "game/core/alpha.cpp", 15) == "opeu_show(ds, x, static_cast<std::uint8_t>(0))"
  assert next(s for s in model.substitutions.values() if s.site.line == 15).past_end == [1]


def test_a_used_result_of_a_void_case_is_zero_and_listed(model):
  assert Replacement(model, "game/core/alpha.cpp", 16) == "(opeu_show(ds, x, static_cast<std::uint8_t>(1)), std::uint16_t{ 0 })"


def test_a_pure_local_of_the_dispatcher_is_inlined(model):
  assert Replacement(model, "game/core/alpha.cpp", 18) == "opeu_host(ds, static_cast<void const*>(port_dialogs_host()))"


def test_the_battle_path_follows_the_reorder_and_the_handler(model):
  assert Replacement(model, "game/map/battle.cpp", 11) == "opeu_set_order(ds, svc_btlwalk, step, element)"
  assert Replacement(model, "game/map/battle.cpp", 12) == "static_cast<std::uint16_t>(opeu_grid_step(ds, step, element))"


def test_callbacks_bound_to_different_code_are_not_substituted(model, tmp_path):
  reviews = report.WriteReviews(model, tmp_path)
  polymorphic = (tmp_path / "review-polymorphic.txt").read_text()
  assert "opeu_same parameter 1: 2 dispatchers ['svc_alpha', 'svc_beta']; its Call sites: {'same target': 1}" in polymorphic
  assert "opeu_shared parameter 1: 2 dispatchers ['svc_alpha', 'svc_gamma']; its Call sites: {'DIFFERENT targets': 1}" in polymorphic
  unresolved = (tmp_path / "review-unresolved.txt").read_text()
  assert "case TWICE of svc_alpha is multi-statement" in unresolved
  assert "no handler linked at 0x01030" in unresolved
  assert reviews["unresolved"] == 7


def test_a_site_in_another_file_than_its_target_gets_the_target_header(model):
  substitution = next(s for s in model.substitutions.values() if str(s.site.file) == "game/core/gamma.cpp")
  assert substitution.includes == ["core/target.hpp"]


def test_apply_refuses_the_checkout():
  with pytest.raises(MainCheckout):
    CheckScratch(ROOT)
  with pytest.raises(MainCheckout):
    CheckScratch(ROOT / "sources")


def test_apply_rewrites_a_scratch_copy(tmp_path):
  tree = tmp_path / "tree"
  shutil.copytree(FIXTURE, tree)
  applied = Apply(Model(tree), "all")
  alpha = (tree / "sources/opeu/game/core/alpha.cpp").read_text()
  assert "auto const hoisted0{ static_cast<std::uint16_t>(opeu_tick(ds)) }; auto const hoisted1{ static_cast<std::uint16_t>(opeu_tick(ds)) }; opeu_pair(ds, hoisted1, hoisted0);" in alpha
  assert "Call(ds, event, TWICE);" in alpha
  assert '#include "core/target.hpp"' in (tree / "sources/opeu/game/core/gamma.cpp").read_text()
  assert "#include <array>" in alpha
  assert applied.replaced == {"game/common/rows.hpp": 0, "game/core/alpha.cpp": 10, "game/core/gamma.cpp": 1, "game/io/input.cpp": 7, "game/map/battle.cpp": 5,
                              "game/map/plan.cpp": 8, "game/units/orders.cpp": 4}
  assert applied.skipped == ["game/common/rows.hpp:19 needs core/rows-panel.hpp, which includes this header (an include cycle); left for a hand edit"]


def test_apply_hoists_before_a_declaration_and_names_each_sites_words_apart(tmp_path):
  tree = tmp_path / "tree"
  shutil.copytree(FIXTURE, tree)
  Apply(Model(tree), "b3")
  alpha = (tree / "sources/opeu/game/core/alpha.cpp").read_text()
  first = ("auto const hoisted0{ static_cast<std::uint16_t>(opeu_tick(ds)) }; auto const hoisted1{ static_cast<std::uint16_t>(opeu_tick(ds)) }; "
           "auto const first{ (opeu_pair(ds, hoisted1, hoisted0), std::uint16_t{ 0 }) };")
  second = ("auto const hoisted0_1{ static_cast<std::uint16_t>(opeu_tick(ds)) }; auto const hoisted1_1{ static_cast<std::uint16_t>(first) }; "
            "opeu_pair(ds, hoisted1_1, hoisted0_1);")
  assert first in alpha and second in alpha


def test_the_dry_run_writes_the_model_plans_and_listings(tmp_path):
  assert rewrite.main(["--tree", str(FIXTURE), "--out", str(tmp_path)]) == 0
  summary = json.loads((tmp_path / "summary.json").read_text())
  assert summary["plans"] == {"b2": 5, "b3": 31}
  assert summary["call_sites"] == 43
  assert summary["unresolved_by_reason"] == {"multi-statement case or handler": 2, "id not served": 2, "service id is a parameter": 1,
                                             "callback bound to dispatchers that reach different code": 1, "no link handler at the address": 1}
  plan = json.loads((tmp_path / "plan-b2.json").read_text())
  assert plan[0]["original"] == "Call(ds, event, BattleRoutine::SET_ORDER, element, step)"
  assert {path.name for path in tmp_path.iterdir()} >= {"model.json", "plan-b2.txt", "plan-b3.json", "review-past-end.txt", "review-order.txt"}


def B1(code: str, path: str = "game/core/snippet") -> str:
  """The B1 rewrite of code in a file at path (a snippet without a C++ suffix asks for no include)."""
  return RewriteFile(SourceFile(PurePosixPath(path), code))[0]


@pytest.mark.parametrize("code, rewritten", [
  ("void f(uint8_t* ds, int x);",                    "void f(Game& game, int x);"),
  ("void f(std::uint8_t* ds);",                      "void f(Game& game);"),
  ("int f(const uint8_t* ds);",                      "int f(Game const& game);"),
  ("int f(std::uint8_t const* ds);",                 "int f(Game const& game);"),
  ("int f(uint8_t ds[65536], int x);",               "int f(Game& game, int x);"),
  ("int f(const uint8_t ds[65537]);",                "int f(Game const& game);"),
  ("using E = int (*)(std::uint8_t* ds, int id);",   "using E = int (*)(Game& game, int id);"),
  ("using E = int (*)(uint8_t*, int);",              "using E = int (*)(Game&, int);"),
  ("void f(int (*event)(uint8_t*, int));",           "void f(int (*event)(Game&, int));"),
  ("int opeu_x(uint8_t*, int) { return 0; }",        "int opeu_x(Game&, int) { return 0; }"),
  ("int g(uint8_t*, int);",                          "int g(uint8_t*, int);"),
  ("struct A { std::uint8_t* ds; };",                "struct A { Game& game; };"),
])
def test_b1_turns_every_segment_parameter_into_the_game(code, rewritten):
  assert B1(code) == rewritten


@pytest.mark.parametrize("code, rewritten", [
  ("x = Data(ds).month; f(ds, 1); (void)ds;",         "x = Data(game).month; f(game, 1); (void)game;"),
  ("auto l{ [ds] { return 1; } };",                   "auto l{ [game] { return 1; } };"),
  ("ds[a + 1] = ds[ds[b]];",                          "Byte(game, a + 1) = Byte(game, Byte(game, b));"),
  ("f(ds + Arg(args, count, 0), 2);",                 "f(game.State().Bytes(Arg(args, count, 0)).data(), 2);"),
  ("f(PORT_DS(w(1)));",                               "f(game.State().Bytes(w(1)).data());"),
  ("struct R { int es, ss, ds; }; y = r.ds + s->ds;", "struct R { int es, ss, ds; }; y = r.ds + s->ds;"),
])
def test_b1_hands_on_the_game_and_views_its_bytes(code, rewritten):
  assert B1(code) == rewritten


def test_b1_includes_what_the_rewritten_file_names():
  assert B1('#include "x.hpp"\nint f(uint8_t* ds) { return Data(ds).month; }\n', "game/core/x.cpp") == \
    '#include "x.hpp"\n#include "common/game.hpp"\nint f(Game& game) { return Data(game).month; }\n'
  assert B1('#pragma once\n#include "x.hpp"\nint f(uint8_t* ds);\n', "game/core/x.hpp") == '#pragma once\n#include "x.hpp"\n#include "common/events.hpp"\nint f(Game& game);\n'
  assert '#include "link/link.hpp"' in B1('#include "x.hpp"\nint f() { return port_call(0, 0, 0, 0); }\n', "game/core/x.cpp")
  assert '#include "link/glue.hpp"' in B1('#include "glue.hpp"\n')


def test_b1_rewrites_the_game_of_a_scratch_copy_and_lists_its_views(tmp_path):
  tree = tmp_path / "tree"
  shutil.copytree(FIXTURE, tree)
  result = B1Pass(SourceTree(tree))
  WriteB1(tree, result)
  assert [line.split(":")[:2] for line in result.Listing("view")] == [["game/map/plan.cpp", "13"]]
  assert [line.split(":")[:2] for line in result.Listing("byte")] == [["game/data/actions.cpp", "12"]]
  assert result.Counts()["parameter"] > 0
  gamma = tree.joinpath("sources", "opeu", "game", "core", "gamma.cpp").read_text()
  assert "void opeu_same(Game& game, HostEvent event) {\n  Call(game, event, SHOW, 1, 2);" in gamma
  assert "Data(std::uint8_t* ds)" in tree.joinpath("sources", "opeu", "game", "common", "data-segment.hpp").read_text()  # HAND: the patch


def test_b4_names_the_routine_behind_each_initial_code_pointer(model):
  tables = B4Model(model)["tables"]
  assert tables == [{"field": "control_actions", "slots": 2, "enum": "ControlAction", "initial": ["opeu_action_one", "NONE"]}]


def TargetOf(model: Model, file: str, line: int):
  return next(model.targets[site.Key()] for site in model.sites if str(site.file) == file and site.line == line)


def test_a_case_forwarding_with_an_out_word_reaches_the_shared_dispatchers_case(model):
  assert Replacement(model, "game/units/orders.cpp", 10) == "opeu_unit_value(ds, unit)"
  assert TargetOf(model, "game/units/orders.cpp", 10).chain == ["svc_unit_orders", "svc_order_handler", "OrderService::UNIT_VALUE"]


def test_a_forward_through_a_constexpr_table_remaps_the_id(model):
  assert Replacement(model, "game/units/orders.cpp", 11) == "static_cast<std::uint16_t>(opeu_unit_side(value))"
  assert TargetOf(model, "game/units/orders.cpp", 11).chain == ["svc_unit_table", "svc_order_handler", "OrderService::UNIT_SIDE"]
  assert TargetOf(model, "game/units/orders.cpp", 12).failure == "svc_unit_table: 0x3 is not handed on (shared[id]==NONE holds)"


def test_a_guard_that_is_an_id_predicate_forwards_the_ids_it_lets_through(model):
  assert Replacement(model, "game/units/orders.cpp", 13) == "opeu_wait_units(ds, static_cast<uint8_t>(side))"
  failure = TargetOf(model, "game/units/orders.cpp", 14).failure
  assert failure == "svc_unit_attack: 0x3 is not handed on ((id>OrderService::UNIT_SIDE&&id!=OrderService::WAIT_UNITS) holds)"


def test_an_assigned_forward_is_followed(model):
  chain = TargetOf(model, "game/units/orders.cpp", 15).chain
  assert chain == ["svc_unit_panel", "svc_unit_orders", "svc_order_handler", "OrderService::UNIT_VALUE"]


def test_local_word_helpers_expand_to_their_expressions(model, tmp_path):
  assert Replacement(model, "game/map/plan.cpp", 34) == "opeu_draw_far(x, y, port_far(segment, buffer))"
  assert Replacement(model, "game/map/plan.cpp", 35) == "opeu_draw_near(x, y, PORT_DS(buffer))"
  assert Replacement(model, "game/map/plan.cpp", 36) == "opeu_compose(port_far(segment, buffer), port_far(segment, 0), x)"
  report.WriteReviews(model, tmp_path)
  near = (tmp_path / "review-near.txt").read_text().splitlines()
  assert near[0] == "game/map/plan.cpp:35 opeu_plan_draw -> opeu_draw_near [svc_cell > svc_plan > PlanService::DRAW_NEAR]: near pointer PORT_DS(buffer)"
  assert len(near) == 4


def test_a_local_of_the_dispatcher_that_reads_a_word_reads_the_sites_word(model):
  assert "opeu_highlight_view(ds, static_cast<uint8_t>(column))" in Replacement(model, "game/map/plan.cpp", 40)


@pytest.mark.parametrize("line, replacement", [
  (40, "(side == 0) ? opeu_highlight_view(ds, static_cast<uint8_t>(column)) : opeu_highlight_overview(ds, static_cast<uint8_t>(column))"),
  (41, "if (column & 7) opeu_draw_far(column, side, port_far(0x100, 0x200)); else opeu_draw_near(column, side, PORT_DS(0x100))"),
  (42, "((column & 7) ? opeu_draw_far(column, side, port_far(0x100, 0x200)) : (opeu_draw_near(column, side, PORT_DS(0x100)), std::uint16_t{ 0 }))"),
  (43, "(column & 7) ? static_cast<void>(opeu_draw_far(column, side, port_far(0x100, 0x200))) : static_cast<void>(opeu_draw_near(column, side, PORT_DS(0x100)))"),
])
def test_a_conditional_service_id_chooses_between_both_calls(model, line, replacement):
  assert Replacement(model, "game/map/plan.cpp", line) == replacement


def test_a_conditional_service_ids_words_with_side_effects_are_evaluated_once_first(model):
  substitution = next(s for s in model.substitutions.values() if str(s.site.file) == "game/map/plan.cpp" and s.site.line == 44)
  assert substitution.hoisted == [(0, "opeu_next_column(ds)")]
  assert substitution.text == "(side == 0) ? opeu_highlight_view(ds, static_cast<uint8_t>(hoisted0)) : opeu_highlight_overview(ds, static_cast<uint8_t>(hoisted0))"


def test_a_service_id_that_is_a_parameter_is_reported(model):
  assert TargetOf(model, "game/map/plan.cpp", 48).failure == "service id is a parameter (service of opeu_plan_pixel)"


def test_a_battle_dispatchers_own_branch_is_a_case(model):
  step = next(s for s in model.substitutions.values() if str(s.site.file) == "game/map/battle.cpp" and s.site.line == 67)
  assert (step.text, step.target.battle) == ("opeu_path_step(ds, 5, 6)", True)
  assert "of svc_battle_guard is multi-statement (2 statements)" in TargetOf(model, "game/map/battle.cpp", 68).failure


def test_a_handler_of_a_local_and_its_use_is_one_statement(model):
  assert Replacement(model, "game/map/battle.cpp", 69) == "static_cast<std::uint16_t>(opeu_ask(ds, 8))"
  assert Replacement(model, "game/map/battle.cpp", 70) == "opeu_mark_cell(ds, yes)"


def test_a_word_array_wrapper_forwards_a_group_without_a_count_and_reads_a_span_of_a_literal_length(model):
  assert Replacement(model, "game/io/input.cpp", 43) == "opeu_fill(ds, std::array<std::uint16_t, 3>{ 7, 8, 0 })"
  assert TargetOf(model, "game/io/input.cpp", 43).chain == ["svc_input", "svc_draw", "1"]


def test_a_pointer_local_of_the_dispatcher_from_a_pure_call_is_inlined(model):
  assert Replacement(model, "game/io/input.cpp", 44) == "opeu_put(ds, static_cast<TextHost*>(port_textout_host()), static_cast<std::uint8_t>('x'))"


def test_a_raw_call_of_a_scalar_event_is_a_site_whose_words_are_named(model):
  assert Replacement(model, "game/io/input.cpp", 45) == "opeu_pending(ds, 6, 5)"


def test_a_const_array_declared_for_the_raw_call_alone_folds_into_it(model):
  substitution = next(s for s in model.substitutions.values() if str(s.site.file) == "game/io/input.cpp" and s.site.line == 62)
  assert substitution.text == "static_cast<std::uint16_t>(opeu_sum(ds, 3, 4))" and substitution.removed is not None


def test_a_raw_call_lends_its_array_to_a_case_that_hands_the_words_on(model):
  assert Replacement(model, "game/io/input.cpp", 64) == "opeu_fill_words(ds, buffer, 4)"


def test_a_countless_service_reads_the_elements_of_the_array_it_is_lent(model):
  assert Replacement(model, "game/io/input.cpp", 66) == "opeu_put(ds, static_cast<TextHost*>(port_textout_host()), static_cast<std::uint8_t>(words[0]))"


def test_apply_deletes_the_declaration_of_a_folded_array(tmp_path):
  tree = tmp_path / "tree"
  shutil.copytree(FIXTURE, tree)
  Apply(Model(tree), "b3")
  text = (tree / "sources/opeu/game/io/input.cpp").read_text()
  assert "pair" not in text and "  if (static_cast<std::uint16_t>(opeu_sum(ds, 3, 4))) opeu_keep(ds);\n" in text
  assert "  std::uint16_t buffer[4]{};\n  opeu_fill_words(ds, buffer, 4);\n" in text


def test_dispatchers_whose_cases_are_the_same_text_over_their_own_names_reach_the_same_code(model):
  assert Replacement(model, "game/io/input.cpp", 82) == "opeu_fill_words(ds, buffer, 2)"
  assert TargetOf(model, "game/io/input.cpp", 82).callbacks == ["svc_other_words", "svc_words"]
