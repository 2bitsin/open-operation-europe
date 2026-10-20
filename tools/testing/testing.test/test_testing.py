"""The tests' game-data helper and the skip rule."""
import sys
from pathlib import Path

import pytest

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))
sys.path.insert(0, str(HERE.parents[1] / 'ci'))
import game_data  # noqa: E402
import skips  # noqa: E402


def test_the_names_are_the_tables_twenty_six():
    names = game_data.data_file_names()
    assert len(names) == 26 and 'SCEDATA' in names and names == sorted(names)


def test_a_directory_lacks_what_it_does_not_hold_in_any_case(tmp_path):
    (tmp_path / 'scedata').write_bytes(b'x')
    missing = game_data.missing_files(tmp_path)
    assert 'SCEDATA' not in missing and len(missing) == 25


def test_a_named_directory_without_the_files_fails(tmp_path, monkeypatch):
    monkeypatch.setenv(game_data.DATA_DIRECTORY_VARIABLE, str(tmp_path))
    with pytest.raises(pytest.fail.Exception):
        game_data.data_directory()


def test_a_missing_file_skips_with_the_one_message(tmp_path):
    with pytest.raises(pytest.skip.Exception, match='^no game data: a golden$'):
        game_data.require_file(tmp_path / 'absent.png', 'a golden')


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
