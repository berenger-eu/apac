#!/usr/bin/env python3
"""Ensure broken transformations cannot produce green snapshot test suites."""
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='apac-snapshot-runner-') as temporary:
    root = Path(temporary)
    build = root / 'build'
    build.mkdir()
    for suite, tool in [('DeclarationSplitter', 'declarationSplitter'), ('TaskGraph', 'taskGraph')]:
        tests = root / suite
        (tests / 'autotests').mkdir(parents=True)
        expected = tests / 'expected'
        if suite == 'TaskGraph':
            expected /= 'case'
        expected.mkdir(parents=True)
        source = 'int main() { return 0; }\n'
        (tests / 'autotests/case.cpp').write_text(source)
        (tests / 'sequential-declarations.hpp').write_text('')
        for graph in ('taskGraphRaw.dot', 'taskGraphOpt.dot'):
            (expected / graph).write_text('digraph G {}\n')
        scenarios = [('valid', source, 0, True, True),
                     ('transform-error', source, 7, True, False),
                     ('invalid-cpp', 'int main() { ??? }\n', 0, True, False),
                     ('changed-source', 'int main() { return 1; }\n', 0, True, False)]
        if suite == 'TaskGraph':
            scenarios.append(('invalid-openmp', 'int main() { int x = 0;\n'
                              '#pragma omp task depend(inout: x[0])\n{x++;} return 0; }\n',
                              0, True, False))
            # Run after graph-producing cases to check that stale graphs are rejected.
            scenarios.append(('missing-graphs', source, 0, False, False))
        for label, output, exit_code, graphs, should_pass in scenarios:
            (expected / 'case.cpp').write_text(output if label in ('invalid-cpp', 'invalid-openmp') else source)
            executable = build / tool
            executable.write_text('#!/usr/bin/env python3\nfrom pathlib import Path\n' +
                                  ("Path('taskGraphRaw.dot').write_text('digraph G {}\\n')\n"
                                   "Path('taskGraphOpt.dot').write_text('digraph G {}\\n')\n" if graphs else '') +
                                  f'print({output!r}, end="")\nraise SystemExit({exit_code})\n')
            executable.chmod(0o755)
            result = subprocess.run([sys.executable, ROOT / 'scripts/snapshot-tests.py', suite,
                                     '--build-dir', build, '--tests-dir', tests],
                                    capture_output=True, text=True, timeout=60)
            assert (result.returncode == 0) == should_pass, (suite, label, result.stdout, result.stderr)
            print(f'PASS: snapshot runner {suite} {label}')
