# Integration validation

Run the complete suite after building APAC:

```sh
bash scripts/run-tests.sh
```

The suite includes `scripts/validation-regression.py`, which exercises TaskGraph
in isolation (after function duplication) and the complete APAC pipeline. Both
outputs are compiled and compared against the original sequential program.
Recursive workers remain subject to transformation; only the small support
routines for initialization, leaves, merging, printing, and checksums are kept
sequential. Thus the tests cover real recursive task creation and joins.

The integration cases cover:

- recursive buffer transformations and merge sort;
- runtime partition boundaries and changing partition layouts after a taskwait;
- shifted pointer aliases referring to the same partition;
- aliases chosen by a conditional and pointer rebinding through a reference;
- alternating integer and floating-point element accesses;
- indirect calls, including callback reassignment between outstanding tasks;
- buffers local to conditional scopes and their lifetime at scope exit.

Each case uses nine deterministic `(size, seed)` inputs, covering sizes
`0, 1, 2, 3, 7, 16, 31, 64`, both conditional branches, odd recursive splits,
and empty partitions. Every run compares the scalar result and all 132 buffer
entries. Entries outside the active range are initialized to sentinels and must
remain untouched. Merge sort also has an independent Python sorting oracle.

Both generated paths are compiled with `-O0`, `-O2`, and `-O1` with UBSan
(`-fsanitize=undefined -fno-sanitize-recover=all`). Each binary runs with
1, 2, 4, and 8 OpenMP workers, three times per input. The default matrix performs
5,832 output comparisons for nine integration cases. UBSan complements the
output comparisons; it does not detect all data races or prove their absence.
No timing threshold or speedup assertion is used in this correctness suite.

For a targeted run or custom build:

```sh
python3 scripts/validation-regression.py --case indirectCalls --keep-workdir
python3 scripts/validation-regression.py --build-dir /path/to/build
```

`APAC_BUILD_DIR` is also supported. `--case` can be repeated and `--repeats`
changes the number of runs per configuration. Failure preserves the working
directory and prints its path, with original and generated sources, compiled
binaries, and expected/actual outputs for mismatches. A successful run with
`--keep-workdir` also retains a JSON summary of the tested matrix.
