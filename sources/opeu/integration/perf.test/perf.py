#!/usr/bin/env python3
"""The performance baseline and the measurements behind the perf gates (test_perf.py and the modules'
`<module>.bench/benches.test.py`).

What is measured:

  runs     every run of the functional suite (play.py's RUNS: the six one-hour demos, the battle-viewed demo, the
           three smokes, the save and load runs; in a Debug build only DEBUG_RUNS). Headless play is virtual time, so a
           run goes as fast as the CPU allows and its time is the game's throughput. A run's cost is the CPU time
           (user + system) of its `opeu` process; the wall time is recorded beside it.
  benches  every google-benchmark case of the `*-benches` executables (BENCH_TARGETS): CPU time per iteration.

`../fixtures/perf/baseline.json` holds, per machine (CPU model, core count, OS family, architecture, Python) and per
build type, the numbers of one revision, and the v1.0.1 runs as the reference the refactor is measured against.

How the gates decide (the evidence is in docs/building.md, "Benchmarks and profiling"):

  slowdown a shared machine runs the game up to a third slower while other builds fill its caches and push processes
           onto the efficiency cores. Every play session therefore interleaves CALIBRATION processes (a fixed
           pure-Python workload, CALIBRATION_WORKLOAD) with the plays in the same pool, so they meet the same load;
           the median of their CPU times against the baseline's is the session's slowdown. It tracked the CPU time
           of v1.0.1's plays to within 5 to 8 % while those moved by 12 to 21 % between sessions. On a
           machine the baseline does not list, the same ratio scales the closest listed machine's numbers.
  a run    passes when the median of RUN_SAMPLES plays is within the budget of the baseline median, times the
           slowdown, plus RUN_SLACK_S (process start-up jitter, which dominates the 50 ms runs); one over it is
           played RUN_RETRY_SAMPLES more times and judged on the median of all. The suite's total is judged too.
  a bench  passes when the fastest of its repetitions is within the budget of the baseline's fastest, times the
           executable's own slowdown: how much slower than at the baseline it runs its Calibration benchmark
           (services.bench/calibration.hpp, a fixed workload no game code touches; on the baseline's machine never
           below 1). The benchmarks over their limit are measured again, BENCH_REPETITIONS at a time, for up to
           BENCH_RETRY_SECONDS; those still over get BENCH_ATTEMPTS more attempts, BENCH_ATTEMPT_PAUSE_S apart,
           each measuring them beside their executable's Calibration benchmark and judging them against the limit
           that calibration gives at that moment (a neighbour job that slows a benchmark for a while slows its
           calibration too). The budget stays the same. Noise only ever slows a benchmark down, so the fastest
           repetition estimates its true cost and a real regression fails every attempt.
  budget   OPEU_PERF_BUDGET percent (default BUDGET_PERCENT) over the baseline of the same machine; on a machine the
           baseline does not list, FOREIGN_BUDGET_PERCENT unless OPEU_PERF_BUDGET says otherwise: one calibration
           ratio cannot capture every difference between two machines.

    python3 perf.py baseline <build dir> [--samples N]      measure a build tree and write its baseline entry
    python3 perf.py reference <opeu> [NAME] [--samples N]   record another build's runs as a reference (v1.0.1)
    python3 perf.py measure <build dir> [--samples N]       print the numbers without writing them
    python3 perf.py add-runs <build dir> [--samples N]      baseline the runs the entry lacks, the rest kept (add_runs)

`<build dir>` is a buildutil build tree (`_build/<profile>`): its `bin/opeu` is the game, and its benchmark
executables are built there (BUILD_BENCHMARKING is configured on for them; buildutil passes the switch on every
configure, so the next `./buildutil build|test` turns it off again).
"""
from __future__ import annotations

import argparse
import fcntl
import json
import os
import platform
import re
import resource
import shutil
import statistics
import subprocess
import sys
import tempfile
import time
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass, field
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / 'play.test'))
import play  # noqa: E402

BASELINE = HERE.parent / 'fixtures' / 'perf' / 'baseline.json'
BENCH_TARGETS = ['opeu-services-benches', 'opeu-game-benches']

BUDGET_PERCENT = 10
FOREIGN_BUDGET_PERCENT = 50
BUDGET_VARIABLE = 'OPEU_PERF_BUDGET'
RUN_SAMPLES = 3
RUN_RETRY_SAMPLES = 3
RUN_SLACK_S = 0.025
CALIBRATION_EVERY = 3  # plays per calibration process in a session's pool
BENCH_REPETITIONS = 5
BENCH_MIN_TIME = '0.02s'  # each repetition runs at least this long
BENCH_RETRY_SECONDS = 20  # how long a bench gate keeps re-measuring the benchmarks over their limit
BENCH_ATTEMPTS = 3  # then: attempts beside a fresh calibration for the benchmarks still over
BENCH_ATTEMPT_PAUSE_S = 5  # the pause before each attempt, to step out of a neighbour job's burst
BASELINE_SAMPLES = 7
# A Debug build plays about 50 times slower than Release (the demos take a minute of CPU each), so there the gate
# times only the runs that stay under 3 s of CPU: enough to see a regression, inside ctest's 60 s for the module.
DEBUG_RUNS = ['smoke-input', 'save-new', 'load-play-save', 'load-save']
# The calibration process: dictionary updates and a sort, about 0.5 s of CPU on an Apple M2. Interpreted code with a
# working set of a few hundred KiB meets the load the way the game does (see `slowdown` above).
CALIBRATION_WORKLOAD = '''
table = {}
for step in range(2_000_000):
    table[step % 5003] = (table.get(step % 4999, 0) * 31 + step) & 0xffffffff
sorted(table.values())
'''


def gated_runs(build_type: str) -> list[str]:
    """The runs the gate times in a build of this type."""
    return list(DEBUG_RUNS) if build_type == 'debug' else list(play.RUNS)


# ---- the machine and the build ----

def machine() -> dict:
    """What identifies the machine class a baseline holds for. The OS is its family (an OS update keeps the entry);
    the Python version is part of it because the calibration is Python."""
    return {'cpu': cpu_model(), 'cores': os.cpu_count() or 0, 'os': platform.system(), 'arch': platform.machine(),
            'python': f'{sys.version_info.major}.{sys.version_info.minor}'}


def cpu_model() -> str:
    if sys.platform == 'darwin':
        return subprocess.run(['sysctl', '-n', 'machdep.cpu.brand_string'], capture_output=True,
                              text=True).stdout.strip()
    cpuinfo = Path('/proc/cpuinfo')
    if cpuinfo.exists():
        names = re.findall(r'^(?:model name|Model|CPU part)\s*:\s*(.+)$', cpuinfo.read_text(), re.M)
        if names:
            return names[0].strip()
    return platform.processor() or platform.machine()


def os_version() -> str:
    """The OS release, recorded beside the numbers for the reader."""
    if sys.platform == 'darwin':
        return f'macOS {platform.mac_ver()[0]}'
    return f'{platform.system()} {platform.release()}'


@dataclass(frozen=True)
class Build:
    """A buildutil build tree: the game binary, the build type and the benchmark executables."""
    root: Path

    @classmethod
    def of_binary(cls, opeu: str | Path) -> 'Build':
        return cls(Path(opeu).resolve().parent.parent)

    @property
    def opeu(self) -> Path:
        return self.root / 'bin' / 'opeu'

    def cache(self, key: str) -> str:
        found = re.search(rf'^{key}:[A-Z]+=(.*)$', (self.root / 'CMakeCache.txt').read_text(), re.M)
        return found.group(1) if found else ''

    @property
    def build_type(self) -> str:
        """'release' or 'debug'; RelWithDebInfo and MinSizeRel share release's baseline."""
        kind = self.cache('CMAKE_BUILD_TYPE').lower() or 'release'
        return 'debug' if kind == 'debug' else 'release'

    def benches(self, targets: list[str] = BENCH_TARGETS) -> list[Path]:
        """Configures the tree with BUILD_BENCHMARKING on and builds the benchmark executables. The bench gates of
        several modules run at once under ctest, so the configure and build hold a lock on the tree."""
        cmake = self.cache('CMAKE_COMMAND') or 'cmake'
        with open(self.root / '.perf-benches.lock', 'w') as lock:
            fcntl.flock(lock, fcntl.LOCK_EX)
            run_checked([cmake, '-DBUILD_BENCHMARKING=ON', str(self.root)])
            run_checked([cmake, '--build', str(self.root), '--target', *targets])
        found = {name: sorted(self.root.rglob(name)) for name in targets}
        missing = [name for name, paths in found.items() if not paths]
        if missing:
            raise RuntimeError(f'the build of {", ".join(missing)} in {self.root} left no executable')
        return [paths[0] for paths in found.values()]


def run_checked(command: list[str]) -> str:
    done = subprocess.run(command, capture_output=True, text=True)
    if done.returncode:
        raise RuntimeError(f'{" ".join(command)} failed ({done.returncode}):\n{done.stdout[-3000:]}{done.stderr[-3000:]}')
    return done.stdout


# ---- runs ----

@dataclass(frozen=True)
class Sample:
    cpu: float  # seconds of user + system time of the opeu process
    wall: float


@dataclass
class Session:
    """The plays of one measurement and the calibration processes that ran beside them."""
    runs: dict[str, list[Sample]]
    calibration: list[float] = field(default_factory=list)  # CPU seconds of each calibration process

    def calibration_median(self) -> float:
        return statistics.median(self.calibration)

    def extend(self, more: 'Session') -> None:
        for name, samples in more.runs.items():
            self.runs.setdefault(name, []).extend(samples)
        self.calibration.extend(more.calibration)


def time_run(opeu: str, name: str, out: str) -> Sample:
    """One play of a run in its own directory. Called in a process of its own (`perf.py sample`), so the children's
    rusage is exactly the run's."""
    before = resource.getrusage(resource.RUSAGE_CHILDREN)
    start = time.perf_counter()
    result = play.run(opeu, name, out)
    wall = time.perf_counter() - start
    after = resource.getrusage(resource.RUSAGE_CHILDREN)
    expected = play.load_goldens().get(name, result)['exit']  # a run without goldens yet: play.test reports it
    if result['exit'] != expected:
        raise RuntimeError(f'{name}: exit {result["exit"]} instead of {expected}, outputs in {out}')
    return Sample(after.ru_utime - before.ru_utime + after.ru_stime - before.ru_stime, wall)


def calibrate() -> float:
    """CPU seconds of one pass of CALIBRATION_WORKLOAD in this process."""
    start = time.process_time()
    exec(compile(CALIBRATION_WORKLOAD, 'calibration', 'exec'), {})
    return time.process_time() - start


def subprocess_job(arguments: list[str]) -> dict:
    done = subprocess.run([sys.executable, str(Path(__file__).resolve()), *arguments], capture_output=True, text=True)
    if done.returncode:
        raise RuntimeError(f'perf.py {" ".join(arguments)} failed:\n{done.stderr[-3000:]}')
    return json.loads(done.stdout)


def session_jobs(opeu: str, names: list[str], rounds: int, scratch: str) -> list[list[str]]:
    """`rounds` plays of each run, a calibration process after every CALIBRATION_EVERY plays."""
    plays = [['sample', opeu, name, str(Path(scratch) / f'{name}-{i}')] for i in range(rounds) for name in names]
    jobs: list[list[str]] = []
    for index, job in enumerate(plays):
        jobs.append(job)
        if index % CALIBRATION_EVERY == CALIBRATION_EVERY - 1 or index == len(plays) - 1:
            jobs.append(['calibrate'])
    return jobs


def default_workers() -> int:
    return max(1, (os.cpu_count() or 2) // 2)


def sample_runs(opeu: Path, names: list[str], rounds: int) -> Session:
    """`rounds` plays of each named run, half the cores' worth at a time, with the calibration processes among them."""
    session = Session({name: [] for name in names})
    with tempfile.TemporaryDirectory(prefix='opeu-perf-') as scratch, \
            ThreadPoolExecutor(max_workers=default_workers()) as pool:
        jobs = session_jobs(str(Path(opeu).resolve()), names, rounds, scratch)
        for job, found in zip(jobs, pool.map(subprocess_job, jobs)):
            if job[0] == 'calibrate':
                session.calibration.append(found['cpu'])
            else:
                session.runs[job[2]].append(Sample(**found))
    return session


def run_summary(samples: list[Sample]) -> dict:
    cpu = [s.cpu for s in samples]
    return {'cpu_median_s': round(statistics.median(cpu), 4), 'cpu_min_s': round(min(cpu), 4),
            'wall_median_s': round(statistics.median(s.wall for s in samples), 4), 'samples': len(samples)}


def session_summary(session: Session) -> dict:
    return {'calibration_cpu_s': round(session.calibration_median(), 4),
            'runs': {name: run_summary(samples) for name, samples in session.runs.items()}}


# ---- benches ----

def sample_benches(executable: Path, repetitions: int, names: list[str] | None = None) -> dict[str, list[float]]:
    """CPU nanoseconds per iteration of the benchmarks of an executable (all, or those named), one value per
    repetition, keyed `<executable>/<benchmark>`."""
    command = [str(executable), '--benchmark_format=json', f'--benchmark_repetitions={repetitions}',
               f'--benchmark_min_time={BENCH_MIN_TIME}']
    if names:
        command.append('--benchmark_filter=^(' + '|'.join(re.escape(n.split('/', 1)[1]) for n in names) + ')$')
    times: dict[str, list[float]] = {}
    for bench in json.loads(run_checked(command))['benchmarks']:
        if bench.get('error_occurred'):
            raise RuntimeError(f'{executable.name}/{bench["run_name"]}: {bench.get("error_message")}')
        if bench.get('run_type') == 'iteration':
            unit = {'ns': 1, 'us': 1e3, 'ms': 1e6, 's': 1e9}[bench['time_unit']]
            times.setdefault(f'{executable.name}/{bench["run_name"]}', []).append(bench['cpu_time'] * unit)
    return times


def sample_all_benches(executables: list[Path], repetitions: int) -> dict[str, list[float]]:
    times: dict[str, list[float]] = {}
    for executable in executables:
        times.update(sample_benches(executable, repetitions))
    return times


CALIBRATION = 'Calibration'  # the benchmark of services.bench/calibration.hpp in every executable


def fastest_benches(executable: Path, standard: 'Standard') -> dict[str, float]:
    """{bench: CPU ns}: the fastest repetition of each benchmark of an executable, the ones over their limit
    re-measured for up to BENCH_RETRY_SECONDS, then given BENCH_ATTEMPTS attempts beside a fresh calibration."""
    fastest = {name: min(v) for name, v in sample_benches(executable, BENCH_REPETITIONS).items()}
    deadline = time.monotonic() + BENCH_RETRY_SECONDS
    while (over := standard.benches_over(fastest)) and time.monotonic() < deadline:
        for name, values in sample_benches(executable, BENCH_REPETITIONS, over).items():
            fastest[name] = min(fastest[name], min(values))
    for _ in range(BENCH_ATTEMPTS):
        if not (over := standard.benches_over(fastest)):
            break
        time.sleep(BENCH_ATTEMPT_PAUSE_S)
        attempt_with_calibration(executable, standard, fastest, over)
    return fastest


def attempt_with_calibration(executable: Path, standard: 'Standard', fastest: dict[str, float], over: list[str]) -> None:
    """Measures the benchmarks `over` beside their calibration benchmark and keeps, for each, its time as judged by
    that calibration: scaled by the session's slowdown over the attempt's, so that `fastest` (whose calibration time
    stays the session's) passes it exactly when the attempt is within the limit of its own moment."""
    calibration = executable.name + '/' + CALIBRATION
    attempt = {name: min(v) for name, v in sample_benches(executable, BENCH_REPETITIONS, [*over, calibration]).items()}
    for name in over:
        scale = standard.machine_slowdown(name, fastest) / standard.machine_slowdown(name, attempt)
        fastest[name] = min(fastest[name], attempt[name] * scale)


def bench_summary(values: list[float]) -> dict:
    return {'cpu_min_ns': round(min(values), 1), 'cpu_median_ns': round(statistics.median(values), 1),
            'samples': len(values)}


# ---- the baseline file ----

def load_baseline() -> dict:
    return json.loads(BASELINE.read_text()) if BASELINE.exists() else {'machines': []}


def save_baseline(baseline: dict) -> None:
    BASELINE.parent.mkdir(parents=True, exist_ok=True)
    BASELINE.write_text(json.dumps(baseline, indent=1, sort_keys=True) + '\n')


def machine_entry(baseline: dict, identity: dict) -> dict:
    """The entry of a machine, created when missing."""
    baseline.setdefault('machines', [])
    for entry in baseline['machines']:
        if entry['machine'] == identity:
            return entry
    entry = {'machine': identity, 'builds': {}, 'reference': {}}
    baseline['machines'].append(entry)
    return entry


def revision(build: Build) -> str:
    """The revision a tree was exported from (`.perf-rev` beside it), else the checkout's HEAD."""
    marker = build.root.parent.parent / '.perf-rev'
    if marker.exists():
        return marker.read_text().strip()[:8]
    return run_checked(['git', '-C', str(HERE), 'rev-parse', '--short=8', 'HEAD']).strip()


def measure(build: Build, rounds: int) -> dict:
    session = sample_runs(build.opeu, gated_runs(build.build_type), rounds)
    executables = build.benches()
    benches: dict[str, list[float]] = {}
    for _ in range(rounds):
        for name, values in sample_all_benches(executables, BENCH_REPETITIONS).items():
            benches.setdefault(name, []).extend(values)
    return {**session_summary(session), 'benches': {name: bench_summary(v) for name, v in sorted(benches.items())}}


def stamp(build: Build) -> dict:
    return {'revision': revision(build), 'measured': time.strftime('%Y-%m-%d'), 'os_version': os_version()}


def record_baseline(build: Build, rounds: int) -> dict:
    baseline = load_baseline()
    entry = machine_entry(baseline, machine())
    entry['builds'][build.build_type] = {**stamp(build), **measure(build, rounds)}
    save_baseline(baseline)
    return entry


def record_reference(opeu: Path, name: str, rounds: int) -> dict:
    baseline = load_baseline()
    entry = machine_entry(baseline, machine())
    entry['reference'][name] = {**stamp(Build.of_binary(opeu)),
                                **session_summary(sample_runs(opeu, list(play.RUNS), rounds))}
    save_baseline(baseline)
    return entry


def add_runs(build: Build, rounds: int) -> dict:
    """Baselines the runs this machine's entry for the build type lacks (a run added to the suite) without re-recording
    the others or the benchmarks. Every gated run is played `rounds` times in one session; the kept runs are the
    session's yardstick: the new runs' numbers are scaled by the kept runs' total CPU median in the baseline over the
    same total now, so they read as if measured beside them. (The calibration processes tell the machine's load, not
    how the game's speed moved since the baseline; the kept runs tell both.) Each kept run's scaled time is printed
    beside its baseline as a check; one far off says the session was uneven and wants a full `baseline` instead."""
    baseline = load_baseline()
    numbers = machine_entry(baseline, machine())['builds'][build.build_type]
    names = gated_runs(build.build_type)
    session = sample_runs(build.opeu, names, rounds)
    kept = [name for name in names if name in numbers['runs']]
    scale = (sum(numbers['runs'][name]['cpu_median_s'] for name in kept) /
             sum(statistics.median(s.cpu for s in session.runs[name]) for name in kept))
    found = {name: scaled_summary(session.runs[name], scale) for name in names}
    for name in kept:
        print(f'{name}: {found[name]["cpu_median_s"]:.4f} s, baseline {numbers["runs"][name]["cpu_median_s"]:.4f} s',
              file=sys.stderr)
    added = {name: {**found[name], **stamp(build)} for name in names if name not in numbers['runs']}
    numbers['runs'] = {name: numbers['runs'].get(name) or added[name] for name in names}
    save_baseline(baseline)
    return {'scale': round(scale, 4), 'added': added}


def scaled_summary(samples: list[Sample], scale: float) -> dict:
    """run_summary of samples whose times are multiplied by `scale`."""
    return run_summary([Sample(s.cpu * scale, s.wall * scale) for s in samples])


# ---- what the gates compare with ----

@dataclass(frozen=True)
class Standard:
    """A build type's baseline numbers as they apply to this machine, with the budget that holds here."""
    numbers: dict
    budget: float   # a fraction: 0.10 allows 10 % over the baseline
    source: str     # which baseline, for the failure messages
    foreign: bool   # the numbers are another machine's

    @classmethod
    def here(cls, build_type: str) -> 'Standard':
        """The standard of this machine: its own entry at BUDGET_PERCENT, else the closest listed machine
        (closest_entry) at FOREIGN_BUDGET_PERCENT. Raises LookupError, from the test that asks and never at
        collection, when no machine has numbers for the build type."""
        identity = machine()
        entries = [e for e in load_baseline().get('machines', []) if build_type in e.get('builds', {})]
        if not entries:
            raise LookupError(f'{BASELINE} has no {build_type} baseline; record one with '
                              f'`python3 perf.py baseline <build dir>` (docs/building.md)')
        same = [e for e in entries if e['machine'] == identity]
        if same:
            return cls(same[0]['builds'][build_type], budget(BUDGET_PERCENT), describe(same[0], build_type), False)
        entry = closest_entry(entries, identity)
        return cls(entry['builds'][build_type], budget(FOREIGN_BUDGET_PERCENT),
                   describe(entry, build_type) + f', scaled to this {identity["cpu"]}', True)

    def run_slowdown(self, session: Session) -> float:
        """How much slower than at the baseline the session's calibration processes ran: the factor every run limit
        is multiplied by (below 1 on a quieter or faster machine)."""
        return session.calibration_median() / self.numbers['calibration_cpu_s']

    def run_limit(self, name: str, slowdown: float) -> float:
        return self.numbers['runs'][name]['cpu_median_s'] * slowdown * (1 + self.budget) + RUN_SLACK_S

    def suite_limit(self, slowdown: float) -> float:
        """The limit of the total of the baselined runs."""
        return sum(r['cpu_median_s'] for r in self.numbers['runs'].values()) * slowdown * (1 + self.budget)

    def bench_limit(self, name: str, fastest: dict[str, float]) -> float:
        """The limit of a benchmark in a session whose times are `fastest`: the baseline's fastest times the
        executable's slowdown on its calibration benchmark."""
        return self.numbers['benches'][name]['cpu_min_ns'] * self.machine_slowdown(name, fastest) * (1 + self.budget)

    def machine_slowdown(self, name: str, fastest: dict[str, float]) -> float:
        """The calibration benchmark's fastest now against the baseline's; on the baseline's own machine never below 1
        (noise only slows it down), elsewhere the ratio as it is (it is the machine's speed)."""
        calibration = name.split('/')[0] + '/' + CALIBRATION
        ratio = fastest[calibration] / self.numbers['benches'][calibration]['cpu_min_ns']
        return ratio if self.foreign else max(1.0, ratio)

    def benches_over(self, fastest: dict[str, float]) -> list[str]:
        """The baselined benchmarks whose time is over their limit."""
        return [n for n, ns in fastest.items()
                if n in self.numbers['benches'] and ns > self.bench_limit(n, fastest)]

    def baselined_benches(self, executable: str) -> list[str]:
        """Every benchmark of an executable the baseline names, the calibration included."""
        return sorted(n for n in self.numbers['benches'] if n.startswith(executable + '/'))


def closest_entry(entries: list[dict], identity: dict) -> dict:
    """The listed machine most like this one: the same architecture first, then the same OS family, then the same
    CPU model, then the nearest core count."""
    def likeness(entry: dict) -> tuple:
        other = entry['machine']
        return (other.get('arch') == identity['arch'], other.get('os') == identity['os'],
                other.get('cpu') == identity['cpu'], -abs(other.get('cores', 0) - identity['cores']))
    return max(entries, key=likeness)


def baselined_names(build_type: str, kind: str) -> list[str]:
    """Every run or benchmark (`kind` 'runs' or 'benches') any machine's baseline of the build type names: what the
    tests are parametrized over, read without measuring anything, so collection never fails on any machine."""
    return sorted({name for entry in load_baseline().get('machines', [])
                   for name in entry.get('builds', {}).get(build_type, {}).get(kind, {})})


def tested_build() -> Build:
    """The build the tests run in: the tree of the `opeu` on PATH (buildutil puts <build>/bin there)."""
    path = shutil.which('opeu')
    assert path, 'opeu is not on PATH (buildutil puts <build>/bin there)'
    return Build.of_binary(path)


def tested_build_type() -> str:
    """The tested build's type for collection; 'release' when no opeu is on PATH (the tests then fail, naming it)."""
    return tested_build().build_type if shutil.which('opeu') else 'release'


def budget(default_percent: float) -> float:
    return float(os.environ.get(BUDGET_VARIABLE, default_percent)) / 100


def describe(entry: dict, build_type: str) -> str:
    numbers = entry['builds'][build_type]
    return f'the {build_type} baseline of {numbers["revision"]} on {entry["machine"]["cpu"]} ({numbers["measured"]})'


def main() -> None:
    if len(sys.argv) == 5 and sys.argv[1] == 'sample':
        sample = time_run(*sys.argv[2:])
        print(json.dumps({'cpu': sample.cpu, 'wall': sample.wall}))
        return
    if sys.argv[1:] == ['calibrate']:
        print(json.dumps({'cpu': calibrate()}))
        return
    parser = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    parser.add_argument('command', choices=['baseline', 'measure', 'reference', 'add-runs'])
    parser.add_argument('target', help='a build tree, or for reference the opeu binary')
    parser.add_argument('label', nargs='?', default='v1.0.1', help='reference: the name it is recorded under')
    parser.add_argument('--samples', type=int, default=BASELINE_SAMPLES)
    args = parser.parse_args()
    target = Path(args.target).resolve()
    if args.command == 'reference':
        found = record_reference(target, args.label, args.samples)
    elif args.command == 'add-runs':
        found = add_runs(Build(target), args.samples)
    elif args.command == 'baseline':
        found = record_baseline(Build(target), args.samples)
    else:
        found = measure(Build(target), args.samples)
    print(json.dumps(found, indent=1))


if __name__ == '__main__':
    main()
