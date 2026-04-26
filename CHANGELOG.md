# Changelog

## [Unreleased]

### Added
- `ParamWriteAnalyzer`: interprocedural read-only pointer parameter analysis in `TaskGraph`.
- 5 new `TaskGraph` test cases: `multipleReturnDep`, `independentCalls`, `ternaryDep`, `structFieldDep`, `whileLoopBody`.
- Python benchmark harness (`benchmarks/benchmark.py`) with `readOnlyParam`, `dotProduct`, and `callChain`.
- Known-failure support in `test.sh` through the `knownFailures` array.

### Fixed
- `multipleExitLabels` test corrected.
- Non-determinism in generated code fixed.

---

## [1.0.1] - 2026-02-01

### Fixed
- CI pipeline stability improvements.
- Removed tests no longer relevant after refactoring.

---

## [1.0.0] - 2025-06-04

### Added
- Multi-pass APAC implementation.
- `DuplicateFunctions`, `ConditionUnstack`, `MultipleDeclSplitter`, `DeclarationSplitter`, `GotoTransfo`, `TaskGraph`, `MainParallel`, `ApacDepth`, `Constify`, `Heapify`, `Unstack`, `Stackheap`.
- Per-transformation test suites with reference outputs.
- `.gitlab-ci.yml` pipeline.
- Docker build environment.
- README with build and usage instructions.

### Changed
- Rewrite from single-pass to multi-pass pipeline architecture.
- Build system migrated to CMake with `find_package(LLVM)`.
