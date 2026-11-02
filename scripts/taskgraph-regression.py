#!/usr/bin/env python3
"""Compare sequential and TaskGraph execution with real OpenMP workers."""
import argparse
import os
import re
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--build-dir', type=Path, default=Path(os.environ.get('APAC_BUILD_DIR', ROOT / 'build')))
args = parser.parse_args()
BUILD = args.build_dir.resolve()


def run(argv, folder, *, threads=1, expected_return=0):
    result = subprocess.run([str(x) for x in argv], cwd=folder,
                            env={**os.environ, 'OMP_NUM_THREADS': str(threads), 'OMP_DYNAMIC': 'FALSE'},
                            capture_output=True, text=True, timeout=45)
    if result.returncode != expected_return:
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
         for name in ('forLoopBody', 'whileLoopBody',
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
# Protected helpers are kept sequential. Independent calls return their
# input rather than an ordering-dependent snapshot of shared state.
cases['protectedHiddenGlobals'] = r'''
#include <mutex>
int state;
std::mutex stateMutex;
int update_protected_apacSeq(int n) {
  std::lock_guard<std::mutex> lock(stateMutex);
  state += n;
  return n;
}
int read_protected_apacSeq() {
  std::lock_guard<std::mutex> lock(stateMutex);
  return state;
}
int indirect(int n) { return update_protected_apacSeq(n); }
int main() {
  int a; a = indirect(1);
  int b; b = indirect(2);
  int result; result = a + b + read_protected_apacSeq();
  return result;
}
'''
cases['explicitGlobalArguments'] = r'''
int counter = 1;
int consume(int n) { return n * 2; }
int produce() { return 9; }
int main() {
  int before; before = consume(counter);
  counter = produce();
  int result; result = before + counter;
  return result;
}
'''
cases['qualifiedGlobalArguments'] = r'''
namespace Data { int counter = 1; }
int consume(int n) { return n * 2; }
int produce() { return 9; }
int main() {
  int counter; counter = 99;
  int before; before = consume(Data::counter);
  Data::counter = produce();
  int result; result = before + Data::counter + counter;
  return result;
}
'''
cases['anonymousGlobalArguments'] = cases['qualifiedGlobalArguments'].replace(
    'namespace Data { int counter = 1; }', 'namespace Data { namespace { int counter = 1; } }')
cases['globalReferenceArgument'] = r'''
int counter = 1;
int consume(const int &n) { return n * 2; }
void replace(int &n) { n = 9; }
int main() {
  int before; before = consume(counter);
  replace(counter);
  int result; result = before + counter;
  return result;
}
'''
cases['globalPointerAlias'] = r'''
int counter = 1;
int main() {
  int *p; p = &counter;
  int before; before = *p;
  *p = 9;
  int result; result = before + counter;
  return result;
}
'''

cases['globalPointerArgument'] = r'''
int counter = 1;
int consume(const int *p) { return *p * 2; }
void replace(int *p) { *p = 9; }
int main() {
  int before; before = consume(&counter);
  replace(&counter);
  int result; result = before + counter;
  return result;
}
'''
cases['globalReferenceAlias'] = r'''
#include <functional>
int counter = 1;
int main() {
  std::reference_wrapper<int> ref(counter);
  ref = counter;
  int before; before = ref;
  ref++;
  int result; result = before + counter;
  return result;
}
'''
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


# Structural assertions complement execution: a correct sequential fallback
# alone must not satisfy regressions for resumed task creation.
resumed_cases = set()
fallback_cases = set()
for kind, loop in {
    'for': 'for (int i = 0; i < 3; ++i) observed += step(i) + x;',
    'while': 'int i = 0; while (i++ < 3) observed += step(i) + x;',
    'do': 'int i = 0; do { observed += step(i) + x; } while (++i < 3);',
}.items():
    name = 'resumeAfter_' + kind
    cases[name] = ("int step(int n) { return n + 1; }\nint main() {\n"
                   "int x; x = 7; int observed; observed = 0;\n" + loop +
                   "\nint afterLoopA; afterLoopA = step(x);\n"
                   "int result; result = observed * 100 + afterLoopA; return result; }\n")
    resumed_cases.add(name)
for flag in (0, 1):
    name = 'resumeAfterConditionalLoop_' + str(flag)
    cases[name] = ("int step(int n) { return n + 1; }\nint main() {\n"
                   "int x; x = 7; int observed; observed = 0;\n"
                   f"int condition; condition = {flag};\n"
                   "if (condition) { for (int i = 0; i < 3; ++i) observed += x; }\n"
                   "int afterLoopA; afterLoopA = step(x);\n"
                   "int result; result = observed * 100 + afterLoopA; return result; }\n")
    resumed_cases.add(name)
for flag in (0, 1):
    name = 'resumeAfterUnbracedConditionalLoop_' + str(flag)
    cases[name] = cases['resumeAfterConditionalLoop_' + str(flag)].replace(
        'if (condition) { for (int i = 0; i < 3; ++i) observed += x; }',
        'if (condition) for (int i = 0; i < 3; ++i) observed += x;')
    resumed_cases.add(name)
cases['resumeAfterSwitchFollowingLoop'] = cases['resumeAfter_for'].replace(
    'int afterLoopA;', 'switch (x) { case 7: observed += 3; break; default: observed += 1; }\nint afterLoopA;')
resumed_cases.add('resumeAfterSwitchFollowingLoop')
cases['resumeAcrossTwoLoops'] = r'''
int step(int n) { return n + 1; }
int main() {
  int x; x = 7;
  for (int i = 0; i < 3; ++i) x += i;
  int afterLoopA; afterLoopA = step(x);
  int observed; observed = 0;
  int i; i = 0;
  while (i++ < 3) observed += afterLoopA;
  int result; result = observed + step(x);
  return result;
}
'''
resumed_cases.add('resumeAcrossTwoLoops')
for count in (0, 1, 3):
    name = 'uncertainLoopAliases_' + str(count)
    cases[name] = ("int step(int n) { return n + 1; }\nint main() {\n"
                   "int a; a = 1; int b; b = 2; int *p; p = &a;\n"
                   f"for (int i = 0; i < {count}; ++i) p = &b;\n"
                   "int afterLoopA; afterLoopA = step(4);\n"
                   "*p = 9; int result; result = a + b + afterLoopA; return result; }\n")
    fallback_cases.add(name)
cases['unknownLoopCallee'] = r'''
extern "C" int abs(int);
int step(int n) { return n + 1; }
int main() {
  int observed; observed = 0;
  for (int i = 0; i < 3; ++i) observed += abs(-i);
  int afterLoopA; afterLoopA = step(observed);
  return afterLoopA;
}
'''
fallback_cases.add('unknownLoopCallee')
cases['transitiveLoopAliasMutation'] = r'''
int a, b;
int *globalPointer;
void rebind() { globalPointer = &b; }
void indirect() { rebind(); }
int step(int n) { return n + 1; }
int main() {
  a = 1; b = 2; globalPointer = &a;
  for (int i = 0; i < 1; ++i) indirect();
  int afterLoopA; afterLoopA = step(4);
  *globalPointer = 9;
  int result; result = a + b + afterLoopA;
  return result;
}
'''
fallback_cases.add('transitiveLoopAliasMutation')

cases['implicitTlsAliasMutation'] = r'''
int a, b;
int *p;
int initialize() { p = &b; return 1; }
thread_local int trigger = initialize();
int step(int n) { return n + 1; }
int main() {
  a = 1; b = 2; p = &a;
  for (int i = 0; i < 1; ++i) (void)trigger;
  int afterLoopA; afterLoopA = step(4);
  *p = 9;
  int result; result = a + b + afterLoopA;
  return result;
}
'''
fallback_cases.add('implicitTlsAliasMutation')
cases['implicitCleanupAliasMutation'] = r'''
int a, b;
int *p;
void rebind(int *unused) { p = &b; }
int step(int n) { return n + 1; }
int main() {
  a = 1; b = 2; p = &a;
  for (int i = 0; i < 1; ++i) {
    int local __attribute__((cleanup(rebind))) = 0;
  }
  int afterLoopA; afterLoopA = step(4);
  *p = 9;
  int result; result = a + b + afterLoopA;
  return result;
}
'''
fallback_cases.add('implicitCleanupAliasMutation')

def check_global_contract(root):
    folder = root / 'global-contract'
    folder.mkdir()
    source = folder / 'input.cpp'
    source.write_text("int state;\nvoid mutate() { state++; state++; }\n"
                      "void indirect() { mutate(); }\n"
                      "int main() { mutate(); indirect(); return 0; }\n")
    run([BUILD / 'taskGraph', source, '--', '-std=c++17'], folder)
    graph = (folder / 'taskGraphRaw.dot').read_text()
    clusters = re.findall(r'subgraph cluster_f\d+ \{\n(.*?)\n\}', graph, re.S)
    assert len(clusters) == 3, graph
    nodes = lambda cluster: set(re.findall(r'^\s+(\d+) \[label=', cluster, re.M))
    edges = re.findall(r'^\s+(\d+) -> (\d+)\[label="([^"]*)"', graph, re.M)
    caller = nodes(clusters[-1])
    assert len(caller) == 2, graph
    assert not any(a in caller and b in caller for a, b, _ in edges), graph
    callee = nodes(clusters[0])
    assert any(a in callee and b in callee and '::state' in label
               for a, b, label in edges), graph
    # Explicit accesses must behave identically for local and global storage.
    bodies = {
        'value': 'int before; before = consume(value); value = produce(); '
                 'int result; result = before + value; return result;',
        'pointer': 'int *p; p = &value; int before; before = *p; *p = 9; '
                   'int result; result = before + value; return result;',
        'reference_alias': 'std::reference_wrapper<int> ref(value); ref = value; '
                           'int before; before = ref; ref++; '
                           'int result; result = before + value; return result;',
        'reference_argument': 'int before; before = consumeRef(value); replace(value); '
                              'int result; result = before + value; return result;',
        'pointer_argument': 'int before; before = consumePtr(&value); replacePtr(&value); '
                            'int result; result = before + value; return result;',
    }
    helpers = ('#include <functional>\nint consume(int n) { return n * 2; } int produce() { return 9; } '
               'int consumeRef(const int &n) { return n * 2; } '
               'void replace(int &n) { n = 9; } '
               'int consumePtr(const int *p) { return *p * 2; } '
               'void replacePtr(int *p) { *p = 9; }\n')
    for name, body in bodies.items():
        graphs = []
        for storage in ('local', 'global'):
            prefix = 'int value = 1;\n' if storage == 'global' else ''
            local = 'int value = 1; ' if storage == 'local' else ''
            source.write_text(prefix + helpers + 'int main() { ' + local + body + ' }\n')
            run([BUILD / 'taskGraph', source, '--', '-std=c++17'], folder)
            graphs.append((folder / 'taskGraphRaw.dot').read_text().replace('::value', 'value'))
        assert graphs[0] == graphs[1], (name, *graphs)
    print('PASS: global contract: explicit accesses and aliases match locals; no hidden effects')

with tempfile.TemporaryDirectory(prefix='apac-taskgraph-regression-') as temporary:
    root = Path(temporary)
    check_global_contract(root)
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
        if name in resumed_cases or name in fallback_cases:
            start = re.search(r'\bint\s+scenario\s*\(\s*\)\s*\{', output)
            assert start, (name, 'missing scenario definition')
            scenario = output[start.start():]
            tail = scenario[scenario.index('int afterLoopA;'):]
            has_tasks = '#pragma omp task ' in tail
            assert has_tasks == (name in resumed_cases), (name, 'unexpected tasking policy', scenario)
            if 'int x;' in scenario:
                # Fusion must not move initialization after a loop that reads x.
                first_loop = min((pos for token in ('for (', 'while (', 'do {')
                                  if (pos := scenario.find(token)) >= 0), default=len(scenario))
                assert scenario.index('x = 7;') < first_loop, (name, 'fusion crossed a loop', scenario)
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
        if name in ('resumeAfter_for', 'resumeAcrossTwoLoops', 'uncertainLoopAliases_1'):
            pipeline_input = folder / 'pipeline.cpp'
            pipeline_source = cases[name]
            pipeline_input.write_text(pipeline_source)
            run([BUILD / 'apac', pipeline_input, '--', '-std=c++17'], folder)
            assert pipeline_input.read_text() == pipeline_source
            for optimization in ('-O0', '-O2'):
                run(['g++', '-std=c++17', '-fopenmp', optimization,
                     folder / 'APACpipeline.cpp', '-o', folder / 'pipeline'], folder)
                for threads in (1, 2, 4):
                    run([folder / 'pipeline'], folder, threads=threads,
                        expected_return=int(expected.strip()) % 256)
            print(f'PASS: full pipeline loop case {name}')
        print(f'PASS: TaskGraph execution {name} (-O0/-O2, 1/2/4 threads)')
print(f'{len(cases)} TaskGraph execution cases passed')
