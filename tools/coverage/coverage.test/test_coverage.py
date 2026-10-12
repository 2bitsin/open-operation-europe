"""The coverage report's parsing of an llvm-cov export, on a fixture (the full run is not part of the suite)."""
import importlib.util
import json
from pathlib import Path

import pytest

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('coverage_tool', HERE.parent / 'coverage.py')
coverage = importlib.util.module_from_spec(spec)
spec.loader.exec_module(coverage)
NAMES = {'menu.cpp:_ZL9CloseMenuv': 'CloseMenu()', '_Z6Volumev': 'Volume()', '_Z8OpenMenuv': 'OpenMenu()'}


@pytest.fixture(scope='module')
def summary():
  return coverage.summarise(json.loads((HERE / 'export.json').read_text()), NAMES)


def test_only_the_programs_sources_count(summary):
  assert list(summary['files']) == ['sources/opeu/game/core/menu.cpp', 'sources/opeu/game/units/move.cpp',
                                    'sources/opeu/services/fm.cpp']


def test_game_directories_are_modules_of_their_own(summary):
  assert list(summary['modules']) == ['game/core', 'game/units', 'services']
  assert coverage.module_of('sources/opeu/opeu/main.cpp') == 'opeu'
  assert coverage.module_of('sources/opeu/contract.hpp') == '(root)'


def test_module_and_total_percentages(summary):
  core = summary['modules']['game/core']
  assert (core['files'], core['lines_covered'], core['lines'], core['line_percent']) == (1, 15, 20, 75.0)
  assert core['function_percent'] == 50.0
  total = summary['total']
  assert (total['lines_covered'], total['lines'], total['functions_covered'], total['functions']) == (25, 60, 2, 6)
  assert total['line_percent'] == 41.7


def test_never_executed_functions_per_module_once_each(summary):
  never = {module: [(r['name'], r['line']) for r in records] for module, records in summary['never_executed'].items()}
  assert never == {'game/core': [('CloseMenu()', 12)], 'services': [('Volume()', 1)]}


def test_routines_ranked_by_unexecuted_lines(summary):
  ranked = [(r['name'], r['unexecuted_lines'], r['executed']) for r in summary['routines']]
  assert ranked == [('Volume()', 20, False), ('CloseMenu()', 3, False)]


def test_a_partly_executed_routine_counts_only_its_dark_lines():
  function = {'regions': [[1, 1, 10, 2, 4, 0, 0, 0], [5, 3, 7, 4, 0, 0, 0, 0], [12, 1, 13, 1, 0, 0, 0, 0],
                          [20, 1, 30, 1, 0, 1, 0, 0]]}
  assert coverage.function_lines(function) == (1, {12, 13})


def test_the_coverage_flags_go_into_the_conan_table():
  text = '[project]\nname = "opeu"\n\n[conan]\noptions_linux = []\n\n[package]\nkind = "none"\n'
  lines = coverage.add_coverage_conf(text).splitlines()
  conan = list(coverage.table_lines(lines, 'conan'))
  assert conan[0].startswith('conf = [')
  assert "&:tools.build:exelinkflags+=['-fprofile-instr-generate', '-fcoverage-mapping']" in conan[0]
  assert conan[1] == 'options_linux = []'
  assert list(coverage.table_lines(lines, 'package')) == ['kind = "none"']


def test_the_coverage_flags_make_a_conan_table_when_there_is_none():
  lines = coverage.add_coverage_conf('[project]\nname = "x"\n').splitlines()
  assert lines[-2] == '[conan]' and lines[-1].startswith('conf = [')


def test_an_existing_conan_conf_is_not_overwritten():
  with pytest.raises(SystemExit):
    coverage.add_coverage_conf('[conan]\nconf = ["x"]\n')


def test_the_compare_table_lists_both_sides():
  before = {'modules': {'a': {'line_percent': 1.0, 'function_percent': 2.0}},
            'total': {'line_percent': 1.0, 'function_percent': 2.0}}
  after = {'modules': {'a': {'line_percent': 3.0, 'function_percent': 4.0}, 'b': {'line_percent': 5.0,
                                                                                  'function_percent': 6.0}},
           'total': {'line_percent': 3.0, 'function_percent': 4.0}}
  assert coverage.compare_table(before, after)[2:] == ['| a | 1.0 % | 3.0 % | 2.0 % | 4.0 % |',
                                                       '| b | 0.0 % | 5.0 % | 0.0 % | 6.0 % |',
                                                       '| **total** | 1.0 % | 3.0 % | 2.0 % | 4.0 % |']


def test_the_report_names_every_section(summary):
  text = coverage.render(summary)
  for heading in ('## Per module', '## Per file', '## Never-executed functions per module', '### services (1)'):
    assert heading in text
