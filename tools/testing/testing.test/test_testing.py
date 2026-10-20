"""The tests' game-data helper (the skip rule that goes with it: test_skips.py)."""
import sys
from pathlib import Path

import pytest

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))
import game_data  # noqa: E402


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


def test_without_any_data_a_run_skips(tmp_path, monkeypatch):
    monkeypatch.delenv(game_data.DATA_DIRECTORY_VARIABLE, raising=False)
    monkeypatch.setattr(game_data, 'IN_TREE_DATA', tmp_path / 'absent')
    with pytest.raises(pytest.skip.Exception, match='^no game data: the 26 data files'):
        game_data.require_data()


def test_a_complete_named_directory_is_the_data(tmp_path, monkeypatch):
    for name in game_data.data_file_names():
        (tmp_path / name.lower()).write_bytes(b'x')
    monkeypatch.setenv(game_data.DATA_DIRECTORY_VARIABLE, str(tmp_path))
    assert game_data.require_data() == tmp_path
