#!/usr/bin/env python3
"""Source-based coverage of what the functional suite's play exercises.

    python3 tools/coverage/coverage.py run [--rev REV | --working-tree] [--reuse] [--runs a,b] [--out DIR]
    python3 tools/coverage/coverage.py report EXPORT.json [--out DIR]
    python3 tools/coverage/coverage.py compare BEFORE.json AFTER.json

`run` exports REV (default HEAD; `--working-tree` copies the checkout's
tracked and untracked-unignored files instead) into `_local/coverage/tree`,
builds it Debug with clang's source-based coverage
(`-fprofile-instr-generate -fcoverage-mapping`, given to the build through a
`[conan] conf` entry added to the export's buildutil.toml, so nothing in
CMake changes), plays every run of
`sources/opeu/integration/play.test/play.py` (the scenario demos, the
battle-viewed demo, the saves and loads and the three smokes) with
`LLVM_PROFILE_FILE` set, merges the profiles with `llvm-profdata` and exports
them with `llvm-cov`. The shared `_pyvenv` and `_conanhome` of the checkout
are reused; nothing in the checkout's own `_build` is touched.

It writes into `_local/coverage/` (or `--out`):
  report.json  per module, per file and per function: lines and functions
               covered, the never-executed functions, the routines with the
               most unexecuted lines
  report.md    the same as tables
  export.json  llvm-cov's raw export (input of `report`)

A module is a directory under sources/opeu/; the game's subdirectories
(game/core, game/units, ...) count as modules of their own. Tests (`*.test*`)
and code outside sources/opeu/ are left out.

`compare` prints the per-module before/after table of two report.json files
(docs/coverage.md holds it and says how to read the results).
"""
import argparse
import importlib.util
import io
import json
import os
import shutil
import subprocess
import sys
import tarfile
import time
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SCRATCH = ROOT / '_local' / 'coverage'
# Where an LLVM toolchain's llvm-profdata, llvm-cov and llvm-cxxfilt live when Xcode's do not apply (Linux clang).
LLVM_DIRS = [Path('/opt/homebrew/opt/llvm/bin'), Path('/usr/local/opt/llvm/bin'), Path('/usr/lib/llvm-18/bin')]
FLAGS = ['-fprofile-instr-generate', '-fcoverage-mapping']
SOURCES = 'sources/opeu/'
TOP_ROUTINES = 40
COUNTS = ('lines', 'lines_covered', 'functions', 'functions_covered')


# --- the tree and its build --------------------------------------------------

def export_revision(rev, tree):
  """`git archive REV` into `tree`, every file stamped now."""
  data = subprocess.run(['git', 'archive', rev], cwd=ROOT, capture_output=True, check=True).stdout
  now = int(time.time())
  with tarfile.open(fileobj=io.BytesIO(data)) as tar:
    members = tar.getmembers()
    for member in members:
      member.mtime = now
    tar.extractall(tree, members=members)


def copy_working_tree(tree):
  """The checkout's tracked and untracked-unignored files, as they are now."""
  listed = subprocess.run(['git', 'ls-files', '-z', '--cached', '--others', '--exclude-standard'], cwd=ROOT,
                          capture_output=True, check=True).stdout.decode().split('\0')
  for name in filter(None, listed):
    source = ROOT / name
    if source.is_file():
      (tree / name).parent.mkdir(parents=True, exist_ok=True)
      shutil.copy2(source, tree / name)


def coverage_conf():
  """The [conan] conf lines that give the consumer (`&:`) the coverage flags."""
  return [f'&:tools.build:{key}+={FLAGS!r}' for key in ('cflags', 'cxxflags', 'exelinkflags', 'sharedlinkflags')]


def table_lines(lines, name):
  """The lines of the TOML table `[name]`."""
  inside = False
  for line in lines:
    stripped = line.strip()
    if stripped.startswith('['):
      inside = stripped == f'[{name}]'
    elif inside:
      yield line


def add_coverage_conf(toml_text):
  """buildutil.toml's text with the coverage conf lines in its [conan] table."""
  lines = toml_text.splitlines()
  entry = 'conf = [' + ', '.join(json.dumps(line) for line in coverage_conf()) + ']'
  if any(line.split('=')[0].strip() == 'conf' for line in table_lines(lines, 'conan')):
    raise SystemExit('buildutil.toml already has [conan] conf; merge the coverage flags into it here')
  stripped = [line.strip() for line in lines]
  if '[conan]' not in stripped:
    return toml_text.rstrip('\n') + '\n\n[conan]\n' + entry + '\n'
  start = stripped.index('[conan]')
  return '\n'.join(lines[:start + 1] + [entry] + lines[start + 1:]) + '\n'


def prepare_tree(tree, rev, working_tree):
  if tree.exists():
    shutil.rmtree(tree)
  tree.mkdir(parents=True)
  if working_tree:
    copy_working_tree(tree)
  else:
    export_revision(rev, tree)
  shutil.copytree(ROOT / '.buildutil', tree / '.buildutil', dirs_exist_ok=True)
  if (ROOT / '.env').exists():
    shutil.copy(ROOT / '.env', tree / '.env')
  toml = tree / 'buildutil.toml'
  toml.write_text(add_coverage_conf(toml.read_text()))


def build(tree, logs):
  """The Debug coverage build of the tree; returns the installed opeu."""
  env = dict(os.environ, BUILDUTIL_VENV_DIR=str(ROOT / '_pyvenv'))
  env.pop('OPEU_LOG', None)
  cmd = [str(tree / 'buildutil'), '--watchdog-budget', '1800', 'build', '--debug', '--no-tests',
         '--conan-home', str(ROOT / '_conanhome')]
  log = logs / 'build.log'
  with open(log, 'w') as out:
    code = subprocess.run(cmd, cwd=tree, env=env, stdout=out, stderr=subprocess.STDOUT).returncode
  if code:
    raise SystemExit(f'coverage build failed (exit {code}); log {log}')
  return tree / '_install' / 'opeu' / 'opeu'


# --- the play ----------------------------------------------------------------

def load_play(tree):
  spec = importlib.util.spec_from_file_location('play', tree / 'sources/opeu/integration/play.test/play.py')
  module = importlib.util.module_from_spec(spec)
  spec.loader.exec_module(module)
  return module


def play_all(tree, binary, out, names):
  """Every run of play.py (or `names`), each process writing its own profile."""
  play = load_play(tree)
  profiles = out / 'profiles'
  shutil.rmtree(profiles, ignore_errors=True)
  profiles.mkdir(parents=True)
  os.environ['LLVM_PROFILE_FILE'] = str(profiles / '%p-%m.profraw')

  def one(name):
    started = time.time()
    result = play.run(binary, name, out / 'runs' / name)
    return name, result['exit'], time.time() - started

  with ThreadPoolExecutor(max_workers=os.cpu_count() or 4) as pool:
    for name, code, seconds in pool.map(one, names or list(play.RUNS)):
      print(f'  {name:24} exit {code}  {seconds:6.1f} s', flush=True)
  return sorted(profiles.glob('*.profraw'))


# --- llvm --------------------------------------------------------------------

def llvm_tool(name):
  """The LLVM tool that reads what the build's compiler wrote: a raw profile's format follows the compiler's LLVM
  version, so Apple clang's profiles need Xcode's tools (`xcrun`), not a newer Homebrew LLVM's."""
  if sys.platform == 'darwin' and subprocess.run(['xcrun', '--find', name], capture_output=True).returncode == 0:
    return ['xcrun', name]
  found = next((directory / name for directory in LLVM_DIRS if (directory / name).exists()), None)
  return [str(found)] if found else [name]


def merge_and_export(binary, profraws, out):
  merged = out / 'merged.profdata'
  subprocess.run(llvm_tool('llvm-profdata') + ['merge', '-sparse', '-o', str(merged)] + [str(p) for p in profraws],
                 check=True)
  export = out / 'export.json'
  with open(export, 'w') as sink:
    subprocess.run(llvm_tool('llvm-cov') + ['export', '-format=text', '-skip-expansions', '-skip-branches',
                                            '-instr-profile', str(merged), str(binary)], stdout=sink, check=True)
  return export


def demangle(names):
  """{symbol: readable name}; a `file.cpp:` prefix (internal linkage) is dropped."""
  bare = [name.rsplit(':', 1)[-1] for name in names]
  try:
    output = subprocess.run(llvm_tool('llvm-cxxfilt'), input='\n'.join(bare), capture_output=True, text=True,
                            check=True).stdout.splitlines()
  except (OSError, subprocess.CalledProcessError):
    output = bare
  return dict(zip(names, output if len(output) == len(bare) else bare))


# --- the summary (pure; tested on a fixture) ---------------------------------

def relative_source(path):
  """'sources/opeu/...' for a source of the program, None for tests and anything else."""
  path = str(path).replace('\\', '/')
  at = path.rfind('/' + SOURCES)
  relative = path[at + 1:] if at >= 0 else (path if path.startswith(SOURCES) else None)
  return None if relative is None or '.test' in relative else relative


def module_of(relative):
  parts = relative[len(SOURCES):].split('/')
  if len(parts) == 1:
    return '(root)'
  if parts[0] == 'game' and len(parts) > 2:
    return f'game/{parts[1]}'
  return parts[0]


def percent(covered, count):
  return round(100.0 * covered / count, 1) if count else 0.0


def function_lines(function):
  """(first line, lines never executed) of a function's code regions in its own file."""
  executed, unexecuted, first = set(), set(), None
  for line_start, _, line_end, _, count, file_id, _, kind in (region[:8] for region in function['regions']):
    if file_id != 0 or kind != 0:
      continue
    first = line_start if first is None else min(first, line_start)
    (executed if count else unexecuted).update(range(line_start, line_end + 1))
  return first, unexecuted - executed


def file_records(data):
  files = {}
  for entry in data['files']:
    relative = relative_source(entry['filename'])
    if relative is None:
      continue
    lines, functions = entry['summary']['lines'], entry['summary']['functions']
    files[relative] = {'module': module_of(relative), 'lines': lines['count'], 'lines_covered': lines['covered'],
                       'functions': functions['count'], 'functions_covered': functions['covered']}
  return files


def function_records(data, names):
  """One record per function (template instances and inline copies merged by name, file and line)."""
  seen, records = set(), []
  for function in data['functions']:
    relative = relative_source(function['filenames'][0])
    if relative is None:
      continue
    first, unexecuted = function_lines(function)
    key = (function['name'].rsplit(':', 1)[-1], relative, first)
    if key in seen:
      continue
    seen.add(key)
    records.append({'name': names.get(function['name'], function['name']), 'file': relative, 'line': first,
                    'unexecuted_lines': len(unexecuted), 'executed': function['count'] > 0})
  return records


def with_percentages(record):
  record['line_percent'] = percent(record['lines_covered'], record['lines'])
  record['function_percent'] = percent(record['functions_covered'], record['functions'])
  return record


def summarise(export, names=None):
  """llvm-cov's export (a dict) -> {'modules', 'files', 'never_executed', 'routines', 'total'}."""
  data = export['data'][0]
  files = file_records(data)
  modules, overall = {}, dict.fromkeys(COUNTS, 0)
  for record in files.values():
    total = modules.setdefault(record['module'], {'files': 0, **dict.fromkeys(COUNTS, 0)})
    total['files'] += 1
    for field in COUNTS:
      total[field] += record[field]
      overall[field] += record[field]
  by_size = lambda r: (-r['unexecuted_lines'], r['file'], r['line'] or 0)
  functions = sorted(function_records(data, names or {}), key=by_size)
  never = {}
  for record in (r for r in functions if not r['executed']):
    never.setdefault(module_of(record['file']), []).append(record)
  return {'modules': {name: with_percentages(m) for name, m in sorted(modules.items())},
          'files': {name: with_percentages(f) for name, f in sorted(files.items())},
          'never_executed': dict(sorted(never.items())),
          'routines': [r for r in functions if r['unexecuted_lines']][:TOP_ROUTINES],
          'total': with_percentages(overall)}


def mangled_names(export):
  return sorted({f['name'] for f in export['data'][0]['functions'] if relative_source(f['filenames'][0])})


# --- the report --------------------------------------------------------------

def module_table(summary):
  rows = ['| Module | Files | Lines | Line % | Functions | Function % |', '|---|---:|---:|---:|---:|---:|']
  for name, m in summary['modules'].items():
    rows.append(f"| {name} | {m['files']} | {m['lines_covered']}/{m['lines']} | {m['line_percent']} "
                f"| {m['functions_covered']}/{m['functions']} | {m['function_percent']} |")
  t = summary['total']
  rows.append(f"| **total** | | {t['lines_covered']}/{t['lines']} | {t['line_percent']} "
              f"| {t['functions_covered']}/{t['functions']} | {t['function_percent']} |")
  return rows


def compare_table(before, after):
  rows = ['| Module | Lines before | Lines after | Functions before | Functions after |',
          '|---|---:|---:|---:|---:|']
  empty = {'line_percent': 0.0, 'function_percent': 0.0}
  for name in sorted(set(before['modules']) | set(after['modules'])) + ['total']:
    b = before['total'] if name == 'total' else before['modules'].get(name, empty)
    a = after['total'] if name == 'total' else after['modules'].get(name, empty)
    label = '**total**' if name == 'total' else name
    rows.append(f"| {label} | {b['line_percent']} % | {a['line_percent']} % "
                f"| {b['function_percent']} % | {a['function_percent']} % |")
  return rows


def render(summary):
  out = ['# Coverage of the functional suite', '', '## Per module', '', *module_table(summary), '',
         f'## The {TOP_ROUTINES} routines with the most unexecuted lines', '',
         '| Routine | File:line | Unexecuted lines | Entered |', '|---|---|---:|---|']
  out += [f"| `{r['name']}` | {r['file']}:{r['line']} | {r['unexecuted_lines']} | {'yes' if r['executed'] else 'no'} |"
          for r in summary['routines']]
  out += ['', '## Per file', '', '| File | Lines | Line % | Functions | Function % |', '|---|---:|---:|---:|---:|']
  out += [f"| {name} | {f['lines_covered']}/{f['lines']} | {f['line_percent']} "
          f"| {f['functions_covered']}/{f['functions']} | {f['function_percent']} |"
          for name, f in summary['files'].items()]
  out += ['', '## Never-executed functions per module', '']
  for module, records in summary['never_executed'].items():
    out += [f'### {module} ({len(records)})', '']
    out += [f"- `{r['name']}` {r['file']}:{r['line']} ({r['unexecuted_lines']} lines)" for r in records]
    out.append('')
  return '\n'.join(out) + '\n'


def write_report(export_path, out):
  export = json.loads(Path(export_path).read_text())
  summary = summarise(export, demangle(mangled_names(export)))
  (out / 'report.json').write_text(json.dumps(summary, indent=1) + '\n')
  (out / 'report.md').write_text(render(summary))
  print('\n'.join(module_table(summary)))
  print(f'report: {out / "report.md"}')
  return summary


# --- the command line --------------------------------------------------------

def arguments(argv):
  parser = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
  commands = parser.add_subparsers(dest='command', required=True)
  run = commands.add_parser('run', help='build with coverage, play, report')
  source = run.add_mutually_exclusive_group()
  source.add_argument('--rev', default='HEAD')
  source.add_argument('--working-tree', action='store_true', help='the checkout as it is, not a commit')
  run.add_argument('--reuse', action='store_true', help='keep the tree from the last run (rebuilds what changed)')
  run.add_argument('--runs', default='', help='comma-separated play.py runs (default: all)')
  run.add_argument('--out', default=str(SCRATCH))
  report = commands.add_parser('report', help='report.md/.json from an llvm-cov export')
  report.add_argument('export')
  report.add_argument('--out', default=str(SCRATCH))
  compare = commands.add_parser('compare', help='per-module before/after table of two report.json files')
  compare.add_argument('before')
  compare.add_argument('after')
  return parser.parse_args(argv)


def main(argv):
  args = arguments(argv)
  if args.command == 'compare':
    before, after = (json.loads(Path(p).read_text()) for p in (args.before, args.after))
    print('\n'.join(compare_table(before, after)))
    return
  out = Path(args.out).resolve()
  out.mkdir(parents=True, exist_ok=True)
  if args.command == 'report':
    write_report(args.export, out)
    return
  tree = out / 'tree'
  if not args.reuse:
    prepare_tree(tree, args.rev, args.working_tree)
  started = time.time()
  binary = build(tree, out)
  print(f'built {binary} in {time.time() - started:.0f} s', flush=True)
  started = time.time()
  profraws = play_all(tree, binary, out, [n for n in args.runs.split(',') if n])
  print(f'played in {time.time() - started:.0f} s, {len(profraws)} profiles', flush=True)
  write_report(merge_and_export(binary, profraws, out), out)


if __name__ == '__main__':
  main(sys.argv[1:])
