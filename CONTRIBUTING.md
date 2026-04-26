# Contributing to APAC

## Development Setup

### Prerequisites

- CMake 3.0+
- LLVM 18 / Clang 18
- GCC or Clang with C++17
- Git

### Using Docker (recommended)

```bash
docker build -t apac .
docker run -v $(pwd):/workspace -it apac /bin/bash
cd /workspace && mkdir -p build && cd build && cmake .. && make -j$(nproc)
```

### Local build

```bash
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

Enable Clang-Tidy static analysis:
```bash
cmake -DENABLE_CLANG_TIDY=ON ..
```

## Running the Tests

Each transformation has its own test suite:

```bash
# Run all test suites
cd /workspace && cd build && make test

# Run a single transformation test suite
cd transforms/TaskGraph/tests && bash test.sh
```

Tests compare the actual APAC output against reference files in `tests/expected/`.
A test is considered passing when the AST, text, and task-graph outputs all match.

Known failures are listed in each `test.sh` with `knownFailures=(...)` but not counted against the exit code, following TDD conventions.

## Coding Standards

- C++17 is the target standard.
- Follow the existing code style (LLVM-based, enforced by `.clang-format`).
- Run `clang-format-18` on modified files before submitting.
- Prefer `RecursiveASTVisitor` patterns consistent with the existing transformation passes.

## Branch and Merge Request Conventions

- `main` - stable, tagged releases only.
- `dev` - integration branch; all feature branches merge here first.
- `feature/<name>` - new features or improvements.
- `fix/<scope>/<name>` - bug fixes.

For changes on the GitLab instance (`git.unistra.fr`):
1. Fork or create a branch from `dev`.
2. Write or update tests for your change.
3. Ensure the full test suite still passes (`bash scripts/run-tests.sh`).
4. Open a merge request targeting `dev` with a descriptive title.

## Adding a New Transformation

1. Create a directory `transforms/<YourTransform>/` following the structure of an existing one.
2. Add `CMakeLists.txt`, `include/`, `src/`, and `tests/` subdirectories.
3. Register your transformation in the root `CMakeLists.txt` and in `transforms/CMakeLists.txt`.
4. Add at least one autotest in `tests/autotests/` with reference output in `tests/expected/`.
5. Integrate the pass into the `apac` pipeline in `cli/src/apac.cpp` if appropriate.

## Adding a New Test Case

1. Create a minimal `.cpp` file in `transforms/<Transform>/tests/autotests/`.
2. Run the binary on your input to generate the reference output:
   ```bash
   cd transforms/TaskGraph/tests/autotests
   /path/to/build/taskGraph mytest.cpp -- > /tmp/mytest.cpp
   clang-format-18 -i /tmp/mytest.cpp
   mkdir -p ../expected/mytest
   cp /tmp/mytest.cpp ../expected/mytest/mytest.cpp
   # Also copy any .dot files produced
   ```
3. If the current output is INCORRECT (TDD), write the expected correct output manually
   and add the test name to `knownFailures` in `test.sh`.
4. Run `bash test.sh` to verify the new test appears and exits cleanly.

## Reporting Bugs

Please open an issue on the project tracker (GitLab) with:
- APAC version or commit hash
- Minimal reproducing input `.cpp` file
- Expected vs. actual output
- Build environment (OS, LLVM version, CMake version)

## Proposing Features

Open an issue describing the use case, the expected transformation, and a minimal
example of input and desired output.
