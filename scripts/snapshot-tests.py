#!/usr/bin/env python3
"""Check transformation snapshots without hiding tool or compiler failures."""
import argparse
import difflib
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('suite', choices=('DeclarationSplitter', 'TaskGraph'))
parser.add_argument('--build-dir', type=Path,
                    default=Path(os.environ.get('APAC_BUILD_DIR', ROOT / 'build')))
parser.add_argument('--tests-dir', type=Path, help=argparse.SUPPRESS)
args = parser.parse_args()
tests = (args.tests_dir or ROOT / 'transforms' / args.suite / 'tests').resolve()
build = args.build_dir.resolve()
tool = 'taskGraph' if args.suite == 'TaskGraph' else 'declarationSplitter'


def command(argv, folder, label, *, stdin=None):
    result = subprocess.run([str(arg) for arg in argv], cwd=folder,
                            input=stdin, capture_output=True, text=True, timeout=60)
    (folder / (label + '.stderr')).write_text(result.stderr)
    (folder / (label + '.stdout')).write_text(result.stdout)
    if result.returncode:
        raise RuntimeError(f'{label}: exit {result.returncode}')
    return result.stdout


def compare(expected, actual, output):
    diff = ''.join(difflib.unified_diff(expected.splitlines(True), actual.splitlines(True),
                                       fromfile='expected', tofile='actual'))
    output.write_text(diff)
    return not diff


passed = failed = 0
inputs = sorted((tests / 'autotests').glob('*.cpp'))
for source in inputs:
    name = source.stem
    folder = tests / 'results' / name
    folder.mkdir(parents=True, exist_ok=True)
    # Graphs must be produced by this invocation, never reused from an earlier run.
    graphs = ('taskGraphRaw.dot', 'taskGraphOpt.dot') if tool == 'taskGraph' else ()
    for graph in graphs:
        (folder / graph).unlink(missing_ok=True)
    errors = []
    try:
        generated = command([build / tool, source, '--', '-std=c++17'], folder, 'transform')
        expected = tests / 'expected' / name / source.name if graphs else tests / 'expected' / source.name
        # Normalize both sides with the same formatter; blank lines are not semantic.
        texts = []
        for label, text in [('expected', expected.read_text()), ('result', generated)]:
            formatted = command(['clang-format-18'], folder, label + '-format', stdin=text)
            path = folder / (label + '.cpp')
            path.write_text(formatted)
            texts.append(''.join(line for line in formatted.splitlines(True) if line.strip()))
        if not compare(*texts, folder / 'source.diff'):
            errors.append('source snapshot')
        asts = []
        for label in ('expected', 'result'):
            flags = ['-std=c++17']
            if graphs:
                # This pass normally runs after DuplicateFunctions. These declarations
                # supply that context for isolated AST snapshots; runtime regressions
                # use real sequential definitions and compile with OpenMP enabled.
                flags += ['-include', tests / 'sequential-declarations.hpp']
            if graphs:
                command(['g++', '-fsyntax-only', '-fopenmp', folder / (label + '.cpp'),
                         *flags], folder, label + '-openmp')
            asts.append(command(['clang-check-18', '-ast-print', folder / (label + '.cpp'),
                                 '--', *flags], folder, label + '-ast'))
        if not compare(*asts, folder / 'ast.diff'):
            errors.append('AST snapshot')
        for graph in graphs:
            actual = folder / graph
            if not actual.is_file():
                errors.append('missing ' + graph)
            elif not compare((expected.parent / graph).read_text(), actual.read_text(),
                             folder / (graph + '.diff')):
                errors.append(graph)
    except (OSError, RuntimeError, subprocess.TimeoutExpired) as error:
        errors.append(str(error))
    if errors:
        failed += 1
        print(f'FAIL: {args.suite} {name}: {", ".join(errors)} (diagnostics: {folder})')
    else:
        passed += 1
        print(f'PASS: {args.suite} {name}')
print(f'{args.suite}: {passed}/{len(inputs)} passed, {failed} failed (no exclusions)')
raise SystemExit(0 if inputs and not failed else 1)
