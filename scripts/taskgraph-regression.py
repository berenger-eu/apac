#!/usr/bin/env python3
"""Compare sequential and TaskGraph execution with real OpenMP workers."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--build-dir', type=Path, default=Path(os.environ.get('APAC_BUILD_DIR', ROOT / 'build')))
args = parser.parse_args()
BUILD = args.build_dir.resolve()


def run(argv, folder, *, threads=1):
    result = subprocess.run([str(x) for x in argv], cwd=folder,
                            env={**os.environ, 'OMP_NUM_THREADS': str(threads), 'OMP_DYNAMIC': 'FALSE'},
                            capture_output=True, text=True, timeout=45)
    if result.returncode:
        raise AssertionError(f'{argv}\n{result.stdout}\n{result.stderr}')
    return result.stdout


wrapper = r'''
#include <cstdio>
int main() {
  int result = -1;
#pragma omp parallel
#pragma omp single
  { result = scenario(); }
  std::printf("%d\n", result);
  return 0;
}
'''
cases = {name: (ROOT / 'transforms/TaskGraph/tests/autotests' / (name + '.cpp')).read_text()
         for name in ('forLoopBody', 'whileLoopBody', 'globalVariable',
                      'multipleWritesSameVar', 'outputParam')}
cases['loopControlFlow'] = r'''
int work(int x) { return x * 3; }
int main() {
  int sum; sum = 0;
  for (int i = 0; i < 6; ++i) {
    if (i == 2) continue;
    sum += work(i);
    if (sum > 20) break;
  }
  int j = 0;
  while (j++ < 3) sum += j;
  do { --j; sum += j; } while (j > 0);
  int values[] = {1, 2, 3};
  for (int value : values) sum += value;
  for (;;) { sum += 2; break; }
  switch (sum % 2) {
  case 0: sum += 4; break;
  default: sum += 1;
  }
  return sum;
}
'''
cases['loopAliasesAndFollowingCode'] = r'''
int main() {
  int a; a = 1;
  int b; b = 2;
  int *p; p = &a;
  int i; i = 0;
  while (i++ < 1) p = &b;
  *p = 9;
  int result; result = a + b;
  if (result > 0) result += 3;
  return result;
}
'''
cases['transitiveGlobalEffects'] = r'''
int state;
int update(int n) { state += n; return state; }
int indirect(int n) { return update(n); }
int main() {
  int a; a = indirect(1);
  int b; b = indirect(2);
  int result; result = a + b + state;
  return result;
}
'''
cases['qualifiedGlobalEffects'] = r'''
namespace Data { int counter; }
int update(int n) { Data::counter += n; return Data::counter; }
int main() {
  int counter; counter = 99;
  int a; a = update(1);
  int b; b = update(2);
  int result; result = a + b + Data::counter + counter;
  return result;
}
'''

cases['anonymousGlobalEffects'] = cases['qualifiedGlobalEffects'].replace(
    'namespace Data { int counter; }', 'namespace Data { namespace { int counter; } }')
cases['wrappedArrayLocators'] = r'''
#include <functional>
int main() {
  int data[2][2] = {};
  int (*ptr)[2] = data;
  std::reference_wrapper<int (*)[2]> values(ptr);
  values[0][0] = 3;
  values[1][1] = 7;
  values[0][0] += values[1][1];
  return values[0][0] + values[1][1];
}
'''

with tempfile.TemporaryDirectory(prefix='apac-taskgraph-regression-') as temporary:
    root = Path(temporary)
    for name, source in cases.items():
        folder = root / name
        folder.mkdir()
        original = folder / 'original.cpp'
        source = source.replace('int main()', 'int scenario()')
        original.write_text(source + wrapper)
        current = folder / 'input.cpp'
        current.write_text(source)
        for tool in ('duplicateFunctions', 'taskGraph'):
            output = run([BUILD / tool, current, '--', '-std=c++17'], folder)
            current = folder / (tool + '.cpp')
            current.write_text(output)
        current.write_text(current.read_text() + wrapper)
        for optimization in ('-O0', '-O2'):
            for label, path in (('original', original), ('transformed', current)):
                run(['g++', '-std=c++17', '-fopenmp', optimization, path,
                     '-o', folder / label], folder)
            expected = run([folder / 'original'], folder)
            for threads in (1, 2, 4):
                for repeat in range(5):
                    actual = run([folder / 'transformed'], folder, threads=threads)
                    assert actual == expected, (name, optimization, threads, repeat, expected, actual)
        print(f'PASS: TaskGraph execution {name} (-O0/-O2, 1/2/4 threads)')
print(f'{len(cases)} TaskGraph execution cases passed')
