# Constify

Constify adds const qualifiers to local variables, parameters and eligible methods.
It emits rewritten source by default; `--in-place` writes changes to input files.
`--verbose` prints the analysis graph and the rewrite plan.

## Analysis

The method graph records mutation evidence. The qualifier analysis in
`transforms/Constify/src/pointer_const_analysis.hpp` tracks independent slots for
an object and each level of its pointee chain. References use the slots of the
referred-to type. Writes, aliases, arguments and unchanged return types impose
constraints; rejection propagates until no slot changes.

A pointer may become constant while its pointee remains mutable. Deep
qualification requires const on intervening pointer levels, including the outer
pointer for a reference. Tests check that references still refer to the original
pointer storage after those conversions. Existing const and volatile qualifiers
are preserved.

External signatures, function addresses, overloads and virtual dispatch constrain
signature changes. Eligible method declarations and definitions in the main file
are updated together. Macro declarations and unsupported declarators are kept
unchanged. The tool does not rewrite return types or declarations in headers.

Before emitting output or overwriting inputs, the command type-checks the
rewritten translation units in memory with the original compilation arguments.
A validation failure returns a nonzero status without writing the inputs.

## Validation

```sh
cmake --build build --target constify -j2
bash transforms/Constify/tests/test.sh
python3 scripts/constify-regression.py
```

The suite compares source and AST snapshots. Semantic cases compile and execute
the original and rewritten programs at O0 and O2, inspect inferred parameter
types, and check idempotence. They cover alias identity, pointer levels, casts,
callbacks, overloads, methods, inherited virtual dispatch, decltype and quicksort.
Use `APAC_BUILD_DIR` for an alternative build directory with the shell suite, or
`--build-dir` with the Python runner.
