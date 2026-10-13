"""The B5x pass's substitutions on small inputs: host arguments and parameters disappear, a forwarded host goes with
them, a host's fields are read from the game, the random draws land on GameState (synced) or its generator, and the
capped arithmetic through a pointer becomes an assignment of a value."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))

from rewriter.b5x import B5xPass, RewriteCapped, RewriteHosts, RewriteRandom  # noqa: E402


def test_host_arguments_go_with_their_comma():
  text = "  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), x, y);\n" \
         "  opeu_screen_reset(game, static_cast<const DialogHost*>(port_dialogs_host()));\n" \
         "  opeu_ui_outline(game, static_cast<UiHost const*>(port_uiprim_host()), 1, 2, 3, 4, 7);\n"
  rewritten, counts = RewriteHosts(text)
  assert rewritten == "  opeu_army_position(game, x, y);\n  opeu_screen_reset(game);\n  opeu_ui_outline(game, 1, 2, 3, 4, 7);\n"
  assert counts["host_argument"] == 3


def test_host_parameters_named_or_not_and_maybe_unused_go():
  text = "void opeu_text_clamp(Game& game, [[maybe_unused]] TextHost* host) {\n" \
         "void opeu_title(Game& game, const UiHost* host, uint16_t x);\n" \
         "void opeu_panel(Game& game, DialogHost const*, uint16_t which);\n"
  rewritten, counts = RewriteHosts(text)
  assert rewritten == "void opeu_text_clamp(Game& game) {\nvoid opeu_title(Game& game, uint16_t x);\nvoid opeu_panel(Game& game, uint16_t which);\n"
  assert counts["host_parameter"] == 3


def test_a_forwarded_host_goes_and_fields_are_read_from_the_game():
  text = "  opeu_text_put(game, host, c);\n  opeu_text_newline(game, host);\n  auto const text{ host->scratch };\n  return host->memory[linear];\n"
  rewritten, counts = RewriteHosts(text)
  assert rewritten == "  opeu_text_put(game, c);\n  opeu_text_newline(game);\n  auto const text{ game.Dialogs().scratch };\n  return game.Ui().memory[linear];\n"
  assert (counts["host_forward"], counts["host_field"]) == (2, 2)


def test_a_host_named_otherwise_is_left_alone():
  text = "  auto const ghost{ host_count + 1 };\n  call(game, hostile);\n"
  assert RewriteHosts(text) == (text, {})


def test_synced_draws_become_game_state_methods_and_direct_ones_the_generators():
  text = '#include "common/random.hpp"\n' \
         "  a = port_rand_bounded(game, 6);\n  b = RandomBelow(game, 100);\n  c = RandomNext(game);\n  RandomSeed(game, ticks);\n" \
         "  d = opeu_rand_bounded(8);\n  opeu_rand_seed(ticks);\n"
  rewritten, counts = RewriteRandom(text)
  assert rewritten == "  a = game.State().RandomBounded(6);\n  b = game.State().RandomBounded(100);\n  c = game.State().RandomNext();\n" \
                      "  game.State().RandomSeed(ticks);\n  d = game.State().Generator().Bounded(8);\n  game.State().Generator().Seed(ticks);\n"
  assert sum(counts.values()) == 7


def test_the_pass_rewrites_game_sources_but_not_the_generator_or_the_glue(tmp_path):
  game = tmp_path / "sources/opeu/game"
  (game / "core").mkdir(parents=True)
  (game / "link").mkdir()
  (game / "core/x.cpp").write_text("  y = port_rand_bounded(game, 3);\n")
  (game / "core/random.cpp").write_text("uint16_t opeu_rand_bounded(uint16_t bound) {\n")
  (game / "link/glue.cpp").write_text("  auto const r{ opeu_rand_bounded(bound) };\n")
  result = B5xPass(tmp_path)
  assert list(result.files) == ["core/x.cpp"]
  assert (game / "core/x.cpp").read_text() == "  y = game.State().RandomBounded(3);\n"
  assert (game / "core/random.cpp").read_text() == "uint16_t opeu_rand_bounded(uint16_t bound) {\n"


def test_capped_arithmetic_through_an_address_becomes_an_assignment_and_a_used_value_keeps_it():
  text = "  opeu_add_byte(1, 99, &general.experience);\n  if (!opeu_sub_byte(1, &strike.hours)) x;\n  opeu_add_word(r, L, stock);\n"
  rewritten, counts, left = RewriteCapped(text)
  assert rewritten == "  general.experience = AddByte(general.experience, 1, 99);\n  if (!(strike.hours = SubByte(strike.hours, 1))) x;\n  opeu_add_word(r, L, stock);\n"
  assert (sum(counts.values()), left) == (2, ["opeu_add_word(r, L, stock)"])
