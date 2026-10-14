"""The B6 pass's substitutions on small inputs: sprintf's destination leaves its word array, both calls' words become
FormatArguments, and a call whose words are a named array is left for the hand pass."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))

from rewriter.b6 import RewriteCasts, RewriteFarText, RewriteFormats, RewriteReserves, RewriteSharedReserves  # noqa: E402


def test_the_destination_leaves_the_words_of_sprintf():
  text = "  opeu_request_format(game, std::array<std::uint16_t, 3>{ text, Near(data, data.format), static_cast<std::uint16_t>(a < b) });\n"
  rewritten, counts, left = RewriteFormats(text)
  assert rewritten == "  opeu_request_format(game, text, std::array<FormatArgument, 2>{ Near(data, data.format), static_cast<std::uint16_t>(a < b) });\n"
  assert (counts["opeu_request_format"], left) == (1, [])


def test_a_destination_with_commas_inside_stays_whole_across_lines():
  text = "  opeu_request_format(game, std::array<uint16_t, 4>{ Pick<std::uint16_t, 2>(game, x), f(a, b),\n" \
         "                                                    'x', \"a,b\" });\n"
  rewritten, _, _ = RewriteFormats(text)
  assert rewritten == "  opeu_request_format(game, Pick<std::uint16_t, 2>(game, x), std::array<FormatArgument, 3>{ f(a, b),\n" \
                      "                                                    'x', \"a,b\" });\n"


def test_printf_keeps_its_words_as_format_arguments_and_a_computed_size():
  text = "  opeu_cursor_print(game, std::array<uint16_t, 1 + sizeof...(words)>{ Near(data, format), static_cast<uint16_t>(words)... });\n"
  rewritten, counts, _ = RewriteFormats(text)
  assert rewritten == "  opeu_cursor_print(game, std::array<FormatArgument, 1 + sizeof...(words)>{ Near(data, format), static_cast<uint16_t>(words)... });\n"
  assert counts["opeu_cursor_print"] == 1


def test_a_computed_size_of_sprintf_shrinks_by_one():
  text = "  opeu_request_format(game, std::array<std::uint16_t, 2 + sizeof...(Words)>{ buffer, format, words... });\n"
  rewritten, _, _ = RewriteFormats(text)
  assert rewritten == "  opeu_request_format(game, buffer, std::array<FormatArgument, 2 + sizeof...(Words) - 1>{ format, words... });\n"


def test_named_word_arrays_are_left_for_the_hand_pass():
  text = "  opeu_request_format(game, words);\n  opeu_cursor_print(game, label);\n"
  rewritten, counts, left = RewriteFormats(text)
  assert (rewritten, counts) == (text, {})
  assert left == ["opeu_request_format(game, words)", "opeu_cursor_print(game, label)"]


def test_far_text_pointers_become_spans_named_or_not():
  text = "uint16_t opeu_advice_x(Game& game, const uint8_t* far_text);\n" \
         "  using Check = uint16_t (*)(Game&, const uint8_t*);\n" \
         "uint8_t opeu_y(Game& game, std::uint8_t const*, uint8_t slot) {\n" \
         "  auto const* bytes{ reinterpret_cast<std::uint8_t const*>(&data) };\n"
  rewritten, counts = RewriteFarText(text)
  assert rewritten == "uint16_t opeu_advice_x(Game& game, std::span<std::uint8_t const> far_text);\n" \
                      "  using Check = uint16_t (*)(Game&, std::span<std::uint8_t const>);\n" \
                      "uint8_t opeu_y(Game& game, std::span<std::uint8_t const>, uint8_t slot) {\n" \
                      "  auto const* bytes{ reinterpret_cast<std::uint8_t const*>(&data) };\n"
  assert counts["far_text"] == 3


def test_word_casts_spanning_an_argument_go_and_partial_ones_stay():
  text = "  opeu_request_format(game, static_cast<std::uint16_t>(text), std::array<FormatArgument, 3>{ static_cast<std::uint16_t>(Near(data, data.f)),\n" \
         "                        static_cast<uint16_t>(a) + 1, static_cast<std::uint16_t>(name) });\n" \
         "  opeu_cursor_print(game, std::array<FormatArgument, 2>{ FORMAT, static_cast<uint16_t>(f(x, y)) });\n"
  rewritten, counts = RewriteCasts(text)
  assert rewritten == "  opeu_request_format(game, text, std::array<FormatArgument, 3>{ Near(data, data.f),\n" \
                      "                        static_cast<uint16_t>(a) + 1, name });\n" \
                      "  opeu_cursor_print(game, std::array<FormatArgument, 2>{ FORMAT, f(x, y) });\n"
  assert counts["word_cast"] == 4


def test_a_reserve_holding_one_text_becomes_a_buffer_and_its_clears_go():
  text = "void f(Game& game) {\n" \
         "  ServiceLocals const locals{ 0x32 }; // the message\n" \
         "  auto&               data{ Data(game) };\n" \
         "  uint16_t const      message{ locals.At(0) };\n" \
         "  locals.Clear(game);\n" \
         "  if (x) { g(message); }\n" \
         "}\n"
  rewritten, counts = RewriteReserves(text)
  assert rewritten == "void f(Game& game) {\n" \
                      "  TextBuffer<0x32> message;\n" \
                      "  auto&               data{ Data(game) };\n" \
                      "  if (x) { g(message); }\n" \
                      "}\n"
  assert counts["text_buffer"] == 1


def test_a_reserve_holding_several_locals_is_left_for_the_hand_pass():
  text = "void f(Game& game) {\n" \
         "  ServiceLocals const locals{ 0x50 };\n" \
         "  auto const          text{ locals.At(0) };\n" \
         "  auto const          name{ locals.At(0x32) };\n" \
         "}\n" \
         "void g(Game& game) {\n" \
         "  ServiceLocals const locals{ 4 };\n" \
         "  auto const          text{ locals.At(0) };\n" \
         "}\n"
  rewritten, counts = RewriteReserves(text)
  assert rewritten == text.replace("  ServiceLocals const locals{ 4 };\n  auto const          text{ locals.At(0) };\n", "  TextBuffer<4> text;\n")
  assert counts["text_buffer"] == 1


def test_a_reserve_holding_several_texts_becomes_one_buffer_under_its_name():
  text = "  ServiceLocals const locals{ 0x50 }; // two texts\n" \
         "  locals.Clear(game);\n" \
         "  auto const          text{ locals.At(0) };\n" \
         "  auto const          name{ locals.At(0x32) };\n"
  rewritten, counts = RewriteSharedReserves(text)
  assert rewritten == "  TextBuffer<0x50> locals; // two texts\n" \
                      "  auto const          text{ locals.At(0) };\n" \
                      "  auto const          name{ locals.At(0x32) };\n"
  assert counts["shared_buffer"] == 1
