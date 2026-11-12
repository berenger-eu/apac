#!/usr/bin/env python3
"""Compile and execute transformed programs to catch semantic regressions."""
import argparse
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--build-dir', type=Path, default=ROOT / 'build')
args = parser.parse_args()
BUILD = args.build_dir.resolve()


def run(command, cwd, *, success=True):
    result = subprocess.run([str(x) for x in command], cwd=cwd,
                            capture_output=True, text=True, timeout=45)
    if success and result.returncode:
        raise AssertionError(f'{command}\n{result.stdout}\n{result.stderr}')
    return result


def equivalent(folder, tool, source):
    original = folder / 'input.cpp'
    output = folder / 'output.cpp'
    original.write_text(source)
    result = run([BUILD / tool, original, '--', '-std=c++17'], folder)
    output.write_text(result.stdout)
    for path in (original, output):
        binary = path.with_suffix('')
        run(['clang++-18', '-std=c++17', path, '-o', binary], folder)
    before = run([original.with_suffix('')], folder)
    after = run([output.with_suffix('')], folder)
    assert (before.stdout, before.returncode) == (after.stdout, after.returncode)


checks = []
with tempfile.TemporaryDirectory(prefix='apac-regression-') as temporary:
    folder = Path(temporary)
    for tool in ('apac', 'apacDepth', 'conditionUnstack', 'constify',
                 'declarationSplitter', 'duplicateFunctions', 'gotoRet',
                 'mainParallel', 'multipleDeclSplitter', 'stackheap',
                 'taskGraph', 'unstack'):
        result = run([BUILD / tool, '--unknown-apac-option'], folder, success=False)
        assert result.returncode > 0, f'{tool}: invalid options should fail without crashing'
    checks.append('invalid command lines: 12 tools')

    equivalent(folder, 'conditionUnstack', '''
int main() {
  int i = 0, total = 0;
  while (int n = ++i < 5 ? i : 0) {
    if (n == 2) continue;
    total += n;
  }
  return !(i == 5 && total == 8);
}
''')
    equivalent(folder, 'conditionUnstack', '''
int main() {
  int i = 0;
  while (int n = ++i < 4 ? i : 0) ++i;
  for (int a = 0, b = 3; a < b; ++a) i += a;
  return !(i == 8);
}
''')
    checks.append('while condition reevaluation, continue, unbraced body, multiple for declarations')

    equivalent(folder, 'unstack', '''
namespace N { int f(int x) { return x + 2; } }
int identity(int x) { return x; }
int nested(int a, int b, int c) { return a * 100 + b * 10 + c; }
int main() {
  typedef int Integer;
  int (*fn)(int) = N::f;
  Integer value = identity(nested(-N::f(1), (N::f(2) + 1) * 2, fn(3)));
  return value != -195;
}
''')
    equivalent(folder, 'unstack', '''
int calls;
int side() { ++calls; return 1; }
int id(int n) { return n; }
int main() {
  int a = id(false && side());
  int b = id(true ? 2 : side());
  return !(a == 0 && b == 2 && calls == 0);
}
''')
    checks.append('Unstack: unary operators, parentheses, qualified and indirect calls, conditional evaluation')

    equivalent(folder, 'constify', '''
int global;
int main() {
  int value = 1;
  int *const p = &value;
  (*p)++;
  int array[2] = {1, 2};
  array[0]++;
  global = value;
  return !(value == 2 && array[0] == 2 && global == 2);
}
''')
    equivalent(folder, 'constify', """
struct Overload {
  int f(int) const { return 1; }
  int f(int) { return 2; }
};
struct Qualifiers {
  int get() noexcept(false) { return 3; }
  int lvalue() & noexcept { return 4; }
};
int overloadedRef(int &n) { return n; }
int overloadedRef(const int &n) { return n; }
struct Separate { int get(int); };
int Separate::get(int n) { return n; }
struct Mutable {
  int value = 0;
  void set() { value = 1; }
  void call() { set(); }
};
int &returnRef(int &n) { return n; }
void takesPointer(int *p) { (void)p; }
int main() {
  int value = 1;
  Qualifiers qualifiers;
  int qualifierResult = qualifiers.get() + qualifiers.lvalue();
  takesPointer(&value);
  returnRef(value) = 2;
  int *p = &value;
  int **pp = &p;
  int **&ref = pp;
  Mutable object;
  object.call();
  Overload overloaded;
  return !(value == 2 && **ref == 2 && object.value == 1 && overloaded.f(0) == 2 &&
           qualifierResult == 7 && overloadedRef(value) == 2);
}
""")
    checks.append('Constify: mutable reference returns, pointer references, overloads, separate declarations and calls on this')

    equivalent(folder, 'gotoRet', """
int select(int n) {
  auto twice = [](int x) { return x * 2; };
  switch (n) {
  case 1: return twice(2);
  case 2: do { return 5; } while (false);
  default: return 6;
  }
}
int single() { int n = 2; return n; }
int main() { return !(select(1) == 4 && select(2) == 5 && single() == 2); }
""")
    checks.append('GotoRet: switch labels, do/while, lambdas and single returns')

    seq = folder / 'seq.cpp'
    seq_source = 'void f_apacSeq(int n) { int local = n; }\n'
    seq.write_text(seq_source)
    result = run([BUILD / 'constify', seq, '--', '-std=c++17'], folder)
    assert result.stdout == seq_source, 'Constify must preserve sequential fallback copies'
    seq.write_text(seq_source + 'int main() { int n = 1; f_apacSeq(n); return 0; }\n')
    result = run([BUILD / 'constify', seq, '--', '-std=c++17'], folder)
    assert seq_source in result.stdout, 'Calls must not cause sequential parameters to be rewritten'

    original = folder / 'unchanged.cpp'
    source = 'int main() { return 0; }\n'
    original.write_text(source)
    result = run([BUILD / 'constify', original, '--', '-std=c++17'], folder)
    assert result.stdout == source, 'Constify must emit source when no edits are needed'
    checks.append('Constify: const pointer to mutable data, arrays, globals and unchanged source')

    equivalent(folder, 'declarationSplitter', """
constexpr int square(int n) { return n * n; }
int main() {
  static constexpr int a = 5, b = square(3);
  static_assert(a + b == 14);
  constexpr int values[] = {a, b};
  constexpr const int &ref = a;
  static_assert(ref == 5 && values[1] == 9);
  int initialized[] = {3, 4};
  int empty[2] = {};
  int (*callback)(int) = square;
  int (*row)[2] = &empty;
  int array[a + b];
  array[13] = 42;
  return array[13] != 42 || initialized[1] != 4 || empty[1] != 0
         || callback(3) != 9 || (*row)[1] != 0;
}
""")
    checks.append('DeclarationSplitter: constexpr values, grouped declarations, arrays, references and complex declarators')

    # Failed parsing must leave an in-place input intact.
    invalid = folder / 'invalid.cpp'
    invalid.write_text('int main() { int x = 1; ??? }\n')
    before = invalid.read_bytes()
    result = run([BUILD / 'constify', '--in-place', invalid, '--', '-std=c++17'], folder, success=False)
    assert result.returncode > 0 and invalid.read_bytes() == before
    result = run([BUILD / 'apac', invalid, '--', '-std=c++17'], folder, success=False)
    assert result.returncode > 0, 'Pipeline must propagate errors'
    assert invalid.read_bytes() == before
    assert 'ConditionUnstackHandler' not in result.stderr, 'Pipeline must stop after a failed pass'
    missing = folder / 'missing.cpp'
    result = run([BUILD / 'apac', missing, '--', '-std=c++17'], folder, success=False)
    assert result.returncode > 0 and not (folder / 'APACmissing.cpp').exists()
    checks.append('in-place parse errors, pipeline failure propagation and missing inputs')

    original = folder / 'pipeline.cpp'
    original.write_text('int f(int x) { return x + 1; }\nint main() { int n = f(f(1)); return n != 3; }\n')
    before = original.read_bytes()
    run([BUILD / 'apac', original, '--', '-std=c++17'], folder)
    assert original.read_bytes() == before, 'Pipeline modified its original input'
    output = folder / 'APACpipeline.cpp'
    assert '__tempVar_' in output.read_text(), 'Unstack must run on the pipeline output'
    binary = folder / 'pipeline'
    run(['g++', '-std=c++17', '-fopenmp', output, '-o', binary], folder)
    run([binary], folder)
    checks.append('full pipeline: unchanged original, nested calls, OpenMP compilation and execution')

    result = run([BUILD / 'paramWriteAnalyzerRegression'], folder)
    checks.append(result.stdout.strip())

for check in checks:
    print(f'PASS: {check}')
print(f'{len(checks)} regression groups passed')
