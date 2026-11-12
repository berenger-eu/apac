#!/usr/bin/env python3
"""Semantic and qualifier-level regressions for the Constify transformation."""
import argparse
import json
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--build-dir', type=Path, default=ROOT / 'build')
args = parser.parse_args()
TOOL = args.build_dir.resolve() / 'constify'


def run(command, folder):
    result = subprocess.run([str(x) for x in command], cwd=folder,
                            capture_output=True, text=True, timeout=45)
    if result.returncode:
        raise AssertionError(f'{command}\n{result.stdout}\n{result.stderr}')
    return result.stdout


def parameters(ast):
    result = {}
    def visit(node):
        if node.get('kind') == 'FunctionDecl' and any(
                child.get('kind') == 'CompoundStmt' for child in node.get('inner', [])):
            result[node['name']] = {child.get('name', ''): child['type']['qualType']
                                    for child in node.get('inner', [])
                                    if child.get('kind') == 'ParmVarDecl'}
        for child in node.get('inner', []):
            visit(child)
    visit(ast)
    return result


CASES = [
    ('indirection levels', r'''
int read(int *p) { return *p; }
void write(int *p) { *p += 1; }
int advance(int *p) { ++p; return *p; }
void leaf(int **p) { **p += 2; }
void reset(int **p, int *next) { *p = next; }
int main() {
  int values[2] = {4, 9};
  int *p = values;
  int *&reference = p;
  write(p);
  p = &values[1];
  if (*reference != 9) return 1;
  leaf(&p);
  reset(&p, values);
  return !(read(p) == 5 && *reference == 5 && advance(values) == 11);
}
''', {'read': {'p': 'const int *const'}, 'write': {'p': 'int *const'},
      'advance': {'p': 'const int *'}, 'leaf': {'p': 'int *const *const'},
      'reset': {'p': 'int **const', 'next': 'int *const'}}),
    ('aliases and conditional pointers', r'''
void assign(int *p) { int *a = p; int *b = a; *b = 17; }
int sum(int *p) { int *a = p; const int *b = a; return *b; }
int main() {
  int x = 1, y = 2;
  int *p = &x;
  int *q = true ? p : &y;
  assign(q);
  return !(x == 17 && sum(p) == 17);
}
''', {'assign': {'p': 'int *const'}, 'sum': {'p': 'const int *const'}}),
    ('reference identity and existing qualifiers', r'''
void mutate(int *p) { *p = 7; }
int main() {
  int a = 1, b = 2;
  int *p = &a;
  int *const &r = p;
  mutate(p);
  p = &b;
  if (*r != 2 || &r != &p) return 1;
  int * const *pp = &p;
  (**pp)++;
  p[0]++;
  return b != 4;
}
''', {}),
    ('references returned by calls', r'''
int *&pointerRef(int *&p) { return p; }
int main() {
  int x = 1, y = 9;
  int *p = &x;
  int *&r = pointerRef(p);
  p = &y;
  *p = 10;
  return !(*r == 10 && &r == &p);
}
''', {}),
    ('virtual dispatch remains unchanged', r'''
struct Base { virtual int value() const { return 1; } };
struct Middle : Base {};
struct Derived : Middle { int value() { return 2; } };
int main() {
  Derived d;
  const Base *p = &d;
  return !(p->value() == 1 && d.value() == 2);
}
''', {}),
    ('fixed declarations, callbacks and overloads', r'''
void external(int *);
void external(int *p) { *p = 6; }
int callback(int *p) { return *p; }
int choose(int *p) { return 1; }
int choose(const int *p) { return 2; }
int main() {
  int x = 1;
  int *p = &x;
  int (*fn)(int *) = callback;
  external(p);
  return !(fn(p) == 6 && choose(p) == 1);
}
''', {'callback': {'p': 'int *const'}}),
    ('arrays, typedefs, storage and anonymous parameters', r'''
typedef int *Pointer;
using DoublePointer = int **;
int array(int values[2]) { return values[0] + values[1]; }
int aliases(Pointer p, DoublePointer pp) { return *p + **pp; }
void unnamed(int *, int **) {}
int main() {
  int data[2] = {2, 3};
  static int *p = data;
  int **pp = &p;
  unnamed(p, pp);
  return !(array(data) == 5 && aliases(p, pp) == 4);
}
''', {'array': {'values': 'const int *const'},
      'aliases': {'p': 'const int *const', 'pp': 'const int *const *const'}}),
    ('explicit cast escape', r'''
int main() {
  int x = 1;
  const int *p = &x;
  int *q = const_cast<int *>(p);
  *q = 9;
  return x != 9;
}
''', {}),
    ('methods, aliases of this and separate definitions', r'''
struct Record {
  int value = 1;
  int array[2] = {1, 2};
  int get() noexcept;
  void aliasWrite() { int &alias = value; ++alias; }
  void arrayWrite() { ++array[0]; }
};
int Record::get() noexcept { return value; }
void touch(Record *p) { p->value = 7; }
struct Child : Record { void throughThis() { touch(this); } };
int main() {
  Record r;
  r.aliasWrite(); r.arrayWrite();
  Child child; child.throughThis();
  return !(r.get() == 2 && r.array[0] == 2 && child.value == 7);
}
''', {}),
    ('subobject addresses, lambdas and default initialization', r'''
struct Data { int value; };
int main() {
  Data data; data.value = 1;
  int *p = &data.value;
  *p = 3;
  int x = 1;
  auto fn = [&x] { x = 7; };
  fn();
  return !(data.value == 3 && x == 7);
}
''', {}),
    ('decltype dependencies', r'''
int main() {
  int x = 1;
  decltype(x) y = x;
  ++y;
  int *p = &x;
  using Pointer = decltype(p);
  Pointer copy = p;
  *copy = 5;
  return !(x == 5 && y == 2);
}
''', {}),
    ('pointer members and nonconst methods', r'''
struct Box { int value; void set() { value = 9; } };
struct Owner { int *saved; void store(int *p) { saved = p; } };
int main() {
  Box box{1}; Box *const p = &box; ++p->value; p->set();
  int x = 1; Owner owner{}; owner.store(&x); *owner.saved = 4;
  return !(box.value == 9 && x == 4);
}
''', {}),
]

# Exercise the repository's sorting example with a small data set at both
# optimization levels; the original fixture allocates 100 million elements.
quicksort = (ROOT / 'transforms/Constify/tests/autotests/test_quicksort.cpp').read_text()
assert '#define SIZE 100000000' in quicksort
CASES.append(('quicksort execution', quicksort.replace('#define SIZE 100000000', '#define SIZE 128'),
              {'swap': {'a': 'int *const', 'b': 'int *const'}}))

with tempfile.TemporaryDirectory(prefix='apac-constify-regression-') as temporary:
    folder = Path(temporary)
    for name, source, expected in CASES:
        original = folder / 'original.cpp'
        output = folder / 'transformed.cpp'
        original.write_text(source)
        transformed = run([TOOL, original, '--', '-std=c++17'], folder)
        output.write_text(transformed)
        # Both optimized and unoptimized executions catch accidental constness
        # of storage subsequently written via an alias or explicit cast.
        for opt in ('-O0', '-O2'):
            for path in (original, output):
                executable = path.with_suffix('')
                run(['clang++-18', '-std=c++17', opt, path, '-o', executable], folder)
                run([executable], folder)
        ast = json.loads(run(['clang++-18', '-std=c++17', '-Xclang',
                              '-ast-dump=json', '-fsyntax-only', output], folder))
        actual = parameters(ast)
        for function, wanted in expected.items():
            for argument, kind in wanted.items():
                assert actual[function][argument] == kind, (
                    name, function, argument, actual[function][argument], kind)
        # A second run must not add qualifiers or change declarators again.
        second = run([TOOL, output, '--', '-std=c++17'], folder)
        assert transformed == second, f'{name}: transformation is not idempotent'
        print(f'PASS: Constify {name}')
print(f'{len(CASES)} Constify semantic cases passed (O0, O2, type checks, idempotence)')
