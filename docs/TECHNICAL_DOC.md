# APAC - Technical Documentation

This document describes APAC's internal architecture and the role of the main transformation passes.

For installation, quickstart, benchmarks, and project-level information, see `README.md`.

---

# 1. Overview

APAC (*Automatic Parallelization of C++*) is a source-to-source compiler based on LLVM/Clang 18.

It takes a sequential C++ file as input and generates C++ code using OpenMP tasks with dependency clauses.

The pipeline follows three main steps:

1. normalize the source code to simplify the analysis;
2. analyze reads, writes, and aliases between statements;
3. generate OpenMP tasks with the appropriate dependencies.

---

# 2. Code organization

```text
apac/
├── cli/                    # Main apac binary
├── core/                   # Shared transformation utilities
├── transforms/             # Transformation passes
│   ├── ApacDepth/          # Recursion depth control, standalone
│   ├── ConditionUnstack/
│   ├── Constify/           # Constant propagation, standalone
│   ├── DeclarationSplitter/
│   ├── DuplicateFunctions/
│   ├── GotoTransfo/
│   ├── Heapify/            # Stack-to-heap conversion, outside main pipeline
│   ├── MainParallel/
│   ├── MultipleDeclSplitter/
│   ├── TaskGraph/
│   └── Unstack/
└── scripts/                # Utility scripts
```

Transformation passes share utilities from `core/`, including AST-to-source conversion, type analysis, variable reference extraction, and common option handling.

---

# 3. Main pipeline

The main `apac` binary applies the following passes in order:

```text
Input C++ source
    ↓
1. DuplicateFunctions
    ↓
2. ConditionUnstack
    ↓
3. MultipleDeclSplitter
    ↓
4. Unstack
    ↓
5. DeclarationSplitter
    ↓
6. GotoTransfo
    ↓
7. TaskGraph
    ↓
8. MainParallel
    ↓
C++ code with OpenMP tasks
```

Available passes not called by the main pipeline:

* `Constify`
* `ApacDepth`
* `Heapify` / `StackHeap`

The `ApacDepth` logic is still used by the main pipeline through `ASTDepthAddVisitor` inside `TaskGraph`.

---

# 4. Pipeline passes

## 4.1 DuplicateFunctions

**Purpose:** create a sequential version of each function with the `_apacSeq` suffix.

These versions are used as fallbacks when the maximum parallelization depth is reached.

Example:

```cpp
int function(int a, int b) {
    return a + b;
}
```

Becomes:

```cpp
int function_apacSeq(int a, int b) {
    return a + b;
}

int function(int a, int b) {
    return a + b;
}
```

---

## 4.2 ConditionUnstack

**Purpose:** move declarations out of `if`, `while`, and `for` conditions.

Example:

```cpp
if (int j = 0; j == 0) {
    ;
}
```

Becomes:

```cpp
{
    int j = 0;
    if (j == 0) {
        ;
    }
}
```

The added block preserves the original scope.

---

## 4.3 MultipleDeclSplitter

**Purpose:** split a multi-variable declaration into separate declarations.

Example:

```cpp
int n = 0, *m = &n, ***p, l = 4, &j = n;
```

Becomes:

```cpp
int n = 0;
int *m = &n;
int ***p;
int l = 4;
int &j = n;
```

This gives later passes one declared variable per statement.

---

## 4.4 Unstack

**Purpose:** extract nested function calls into temporary variables.

Example:

```cpp
int a = g(f(f(b, 2), g(4) + f(7, 8) - g(b)) * 4);
```

Simplified output:

```cpp
int __tempVar_5 = g(4);
int __tempVar_3 = g(b);
int __tempVar_4 = f(7, 8);
int __tempVar_2 = f(b, 2);
int __tempVar_1 = f(__tempVar_2, __tempVar_5 + __tempVar_4 - __tempVar_3);
int a = g(__tempVar_1 * 4);
```

This lets `TaskGraph` analyze dependencies between intermediate calls.

---

## 4.5 DeclarationSplitter

**Purpose:** separate variable declarations from initializations.

Example:

```cpp
int a = 5;
```

Becomes:

```cpp
int a;
a = 5;
```

This lets `TaskGraph` distinguish variable creation from the first write.

The pass also inserts support code for some uninitialized reference cases, including `invalid_ref()`.

---

## 4.6 GotoTransfo

**Purpose:** rewrite function exits to use a single exit point.

Example:

```cpp
int main() {
    return 4;
}
```

Simplified output:

```cpp
#include "_apac_header.hpp"

int main() {
    wrapper_t<int> __result;
    {
        __result = build_wrapper<int>(4);
        goto __exit0;
    }
    __exit0:;
    return *__result;
}
```

`wrapper_t<T>` stores the return value before reaching the exit label.

This pass makes it possible to insert synchronization before the function actually returns.

---

## 4.7 TaskGraph

**Purpose:** analyze dependencies between statements and generate OpenMP tasks.

This is the central APAC pass.

It generates:

* `#pragma omp task` directives;
* `depend(in: ...)`, `depend(out: ...)`, and `depend(inout: ...)` clauses;
* `taskgroup` blocks;
* recursion depth control;
* `.dot` dependency graphs before and after optimization.

## Dependency analysis

For each statement, `TaskGraph` identifies variables that are read, variables that are written, and possible aliases.

Example:

```cpp
a = b + c;
```

Analysis:

```text
Read  : b, c
Write : a
```

Pointer example:

```cpp
*p = 5;
```

Analysis:

```text
Read  : p
Write : *p
```

## OpenMP generation

Input code:

```cpp
int main() {
    int a;
    a = 4;
    int *p;
    p = &a;
    (*p)++;
    a++;
    return 0;
}
```

Simplified output:

```cpp
int main() {
    int __apac_depth_local = __apac_depth;
    int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);

    if (__apac_depth_ok) {
#pragma omp taskgroup
        {
            int a;
#pragma omp task default(shared) depend(inout : a)
            { a = 4; }

            int *p;
#pragma omp task default(shared) depend(inout : p)
            { p = &a; }

#pragma omp task default(shared) depend(inout : a) depend(in : p)
            {
                (*p)++;
                a++;
            }
        }
        return 0;
    } else {
        return main_apacSeq();
    }
}
```

## Recursion depth control

`TaskGraph` adds global variables to limit parallel recursion depth:

```cpp
const int nb_cores = omp_get_max_threads();
const int parallel_depth = ffs(nb_cores);
int __apac_depth = 0;
#pragma omp threadprivate(__apac_depth)
const static int __apac_depth_max = parallel_depth;
```

For recursive calls inside a task, the depth is incremented:

```cpp
__apac_depth = __apac_depth_local + 1;
```

When the maximum depth is reached, APAC calls the sequential `_apacSeq` version.

## ParamWriteAnalyzer

`ParamWriteAnalyzer` determines whether pointer or reference parameters are read-only or may be modified by a function.

Example:

```cpp
long reduce(int *data, int n) {
    long result = 0;
    for (int i = 0; i < n; i++) {
        result += data[i];
    }
    return result;
}
```

Here, `data` is only read. APAC can generate a `depend(in: data)` clause instead of `depend(inout: data)`.

Handled cases include:

* direct writes: `*param = ...`;
* increments: `(*param)++`;
* array writes: `param[i] = ...`;
* passing the parameter to another function that may modify it.

## Main data structures

### `AliasTable`

Tracks relationships between variables, pointers, and references.

Example:

```cpp
int a = 5;
int *p = &a;
*p = 10;
```

`AliasTable` connects `*p` to `a`.

### `Instruction`

Represents a statement analyzed by `TaskGraph`.

Important fields:

```text
instructionString       # Source text of the statement
complexInstruction      # Statement requiring special handling
noFusion                # Prevents fusion with another instruction
dependencies            # Read/write variables
curAliases              # Aliases discovered in the statement
scopedInstructions      # Nested statements
```

### `Graph`

Represents the dependency graph.

* a node represents one or more statements;
* an edge represents a data dependency.

The graph can then be optimized by instruction fusion.

---

## 4.8 MainParallel

**Purpose:** add the OpenMP parallel region around the transformed code in `main`.

APAC inserts:

```cpp
#pragma omp parallel num_threads(nb_cores)
#pragma omp master
```

The master thread starts execution, and tasks created inside the parallel region can be executed by the other OpenMP threads.

---

# 5. Passes outside the main pipeline

## 5.1 ApacDepth

`ApacDepth` is a standalone binary for adding recursive depth control.

In the main pipeline, this logic is already integrated into `TaskGraph`.

## 5.2 Constify

`Constify` detects variables that are never modified and can mark them as `const`.

This pass is not called by the main pipeline.

## 5.3 Heapify / StackHeap

`Heapify` converts some local stack-allocated arrays into heap allocations.

Example:

```cpp
int tab[10];
```

Simplified output:

```cpp
int *apacMemeBloc__tab_0 = new int[10];
int *&tab = apacMemeBloc__tab_0;
```

A `delete[]` is inserted before leaving the scope.

This pass is currently outside the main pipeline.

---

# 6. Shared modules

## 6.1 `core/common.hpp` and `core/common.cpp`

These files contain utilities shared by the transformation passes.

Main categories:

```text
AST-to-source conversion
Type analysis
Expression analysis
Array access handling
Pointer and reference handling
```

Example functions:

```text
getStmtAsString()
getExprAsString()
getCompleteVarDeclStr()
isPointerQualType()
isReferenceQualType()
getPtrDepthAccess()
getAllDeclRefExprInsideExpr()
getArraySubscripts()
```

## 6.2 `APACRecursiveASTVisitor.hpp`

Base class used by APAC visitors.

It centralizes:

* filtering with `-functions`;
* exclusion with `-ignore`;
* main function selection with `-main`;
* automatic skipping of `_apacSeq` functions.

## 6.3 `transfoCommon.hpp`

Contains the common option parsing logic used by transformations.

Handled options:

```text
-main
-functions
-ignore
```

---

# 7. Internal concepts

## 7.1 Aliasing

Two expressions can refer to the same memory location.

Example:

```cpp
int a = 5;
int *p = &a;
*p = 10;
```

Writing to `*p` also writes to `a`.

This information is required to generate correct OpenMP dependencies.

## 7.2 Dereference depth

Example:

```cpp
int a;
int *p = &a;
int **pp = &p;
```

Accesses:

```text
a     -> depth 0
*p    -> depth 1
**pp  -> depth 2
```

The depth helps determine which variable is actually read or written.

## 7.3 Thread-private variables

```cpp
int __apac_depth = 0;
#pragma omp threadprivate(__apac_depth)
```

Each thread has its own copy of `__apac_depth`.

This variable tracks recursion depth per thread.

## 7.4 Instruction fusion

APAC can group multiple statements into the same task to reduce OpenMP task creation overhead.

Example:

```cpp
c = h(a, b);
d = c + a;
```

These two statements can be fused if no intermediate statement can use `c` in parallel.

---

# 8. Known limitations

## 8.1 Global variables

Global variables are not correctly integrated into the dependency analysis.

This can lead to data races when several tasks access the same mutable global variable.

## 8.2 `while` loops

Some `while` loop cases may produce incorrect OpenMP pragma placement.

## 8.3 Structures

Struct field accesses are handled conservatively.

Example:

```cpp
s.x = 1;
s.y = 2;
```

APAC may treat both accesses as depending on the whole `s` object, even if the fields are distinct.

## 8.4 Advanced C++ features

Support is partial for:

* complex templates;
* lambdas;
* exceptions;
* virtual methods;
* function pointers;
* advanced object-oriented code.

## 8.5 External calls

If the body of a called function is not available, APAC cannot analyze its effects precisely.

The call must then be handled conservatively.

## 8.6 I/O operations

Operations such as `std::cout` or `printf` may require manual synchronization.

---

# 9. Tests and validation

Tests are not detailed here to avoid duplicating `README.md` and maintaining the same information in two places.

Each transformation has its own tests under:

```text
transforms/<Transformation>/tests/
```

For the global test command, see `README.md`.

Known failures should be documented either in the limitations section or in the relevant test files.

---

# 10. Debugging

Print an AST node:

```cpp
s->dump();
```

Print the source code attached to a statement:

```cpp
llvm::errs() << getStmtAsString(s, TheRewriter.getLangOpts()) << "\n";
```

Dump the AST of a file:

```bash
clang -Xclang -ast-dump -fsyntax-only file.cpp
```

Check generated code:

```bash
clang-check-18 output.cpp
```

Format generated code:

```bash
clang-format-18 -i output.cpp
```

---

# 11. Adding a transformation

This section only covers APAC-specific internal steps. General contribution rules are in `CONTRIBUTING.md`.

## 11.1 Create the directory structure

```bash
mkdir -p transforms/MyTransform/{src,include,tests/{autotests,expected}}
```

## 11.2 Create the visitor

Minimal example:

```cpp
#include "common.hpp"

class ASTMyTransformVisitor
    : public APACRecursiveASTVisitor<ASTMyTransformVisitor> {
public:
    ASTMyTransformVisitor(Rewriter &R,
                          std::string &mainRef,
                          std::vector<std::string> &functionsRef,
                          std::vector<std::string> &functionsToIgnoreRef)
        : APACRecursiveASTVisitor(R, mainRef, functionsRef, functionsToIgnoreRef) {}

    bool VisitStmt(Stmt *s) {
        // Transformation logic
        return true;
    }
};
```

## 11.3 Add the handler

Create:

```text
transforms/MyTransform/include/myTransform.hpp
transforms/MyTransform/src/myTransform.cpp
```

## 11.4 Register the transformation

Update the files required by the build system and the pipeline:

```text
cli/include/apac.hpp
cli/src/apac.cpp
transforms/CMakeLists.txt
transforms/MyTransform/CMakeLists.txt
```

## 11.5 Add tests

Create the test files under:

```text
transforms/MyTransform/tests/
```

Expected outputs should be formatted with `clang-format-18`.
