"""The M2 rewriter on its fixture tree: the dispatch model, each substitution rule, the review listings, the scratch
apply and the B1/B4 models."""
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
from rewriter.stubs import B1Model, B4Model  # noqa: E402
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
  assert reviews["unresolved"] == 3


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
  assert applied.replaced == {"game/core/alpha.cpp": 8, "game/core/gamma.cpp": 1, "game/map/battle.cpp": 2}
  assert not applied.skipped


def test_the_dry_run_writes_the_model_plans_and_listings(tmp_path):
  assert rewrite.main(["--tree", str(FIXTURE), "--out", str(tmp_path)]) == 0
  summary = json.loads((tmp_path / "summary.json").read_text())
  assert summary["plans"] == {"b2": 2, "b3": 9}
  assert summary["call_sites"] == 14
  plan = json.loads((tmp_path / "plan-b2.json").read_text())
  assert plan[0]["original"] == "Call(ds, event, BattleRoutine::SET_ORDER, element, step)"
  assert {path.name for path in tmp_path.iterdir()} >= {"model.json", "plan-b2.txt", "plan-b3.json", "review-past-end.txt", "review-order.txt"}


def test_b1_counts_the_data_segment_parameters_and_raw_views(model):
  b1 = B1Model(model)
  assert [line.split(":")[:2] for line in b1["raw"]] == [["game/data/actions.cpp", "12"]]
  assert b1["counts"]["data"] == 1


def test_b4_names_the_routine_behind_each_initial_code_pointer(model):
  tables = B4Model(model)["tables"]
  assert tables == [{"field": "control_actions", "slots": 2, "enum": "ControlAction", "initial": ["opeu_action_one", "NONE"]}]
