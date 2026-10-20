"""The skip rule (skips.py): which skips a log holds, and that every skip in the sources goes through the game-data
helpers."""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))
import skips  # noqa: E402

LOG = """
The following tests did not run:
\t 12 - ShippedData.AreTheFilesTheCompiledTableDescribes (Skipped)
\t 40 - ClassicPackForms.NameEachImageForWhatItShows (Skipped)
pytest opeu-integration-pytest: 3 passed, 80 skipped
pytest opeu-game-pytest: 9 passed
"""


def test_the_log_names_every_skip():
    assert skips.skipped(LOG) == ['ctest ShippedData.AreTheFilesTheCompiledTableDescribes', 'ctest ClassicPackForms.NameEachImageForWhatItShows',
                                  'pytest opeu-integration-pytest: 80 skipped']
    assert skips.skipped('pytest opeu-game-pytest: 9 passed\n100% tests passed') == []


SKIP = 'pytest.' + 'skip'  # spelt apart, so this file is no stray skip itself


def test_a_skip_must_go_through_the_helpers():
    files = {'sources/opeu/x/x.test/a.cpp': 'if (!golden) GTEST_SKIP() << NoGameData("golden");\nGTEST_SKIP() << "flaky";\n',
             'tools/y/y.test/test_b.py': f'{SKIP}("later")\n@{SKIP}if(True, reason="x")\n',
             'tools/testing/game_data.py': f'{SKIP}(no_game_data(what))\n'}
    assert skips.stray_skips(files) == ['sources/opeu/x/x.test/a.cpp:2: GTEST_SKIP() << "flaky";', f'tools/y/y.test/test_b.py:1: {SKIP}("later")',
                                        f'tools/y/y.test/test_b.py:2: @{SKIP}if(True, reason="x")']


def test_the_tree_skips_only_through_the_helpers():
    assert skips.stray_skips(skips.tracked_sources()) == []
